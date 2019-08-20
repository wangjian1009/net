#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/utils/time_utils.h"
#include "net_schedule.h"
#include "net_timer.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request_cache.h"

net_log_thread_t net_log_thread_create(net_log_schedule_t schedule, const char * name, net_log_thread_processor_t processor) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    net_log_thread_t log_thread = mem_alloc(schedule->m_alloc, sizeof(struct net_log_thread) + (processor ? processor->m_capacity : 0));

    log_thread->m_schedule = schedule;
    log_thread->m_name = name;
    log_thread->m_state = net_log_thread_state_stoped;
    
    if (processor) {
        log_thread->m_processor = *processor;
    }
    else {
        bzero(&log_thread->m_processor, sizeof(log_thread->m_processor));
    }
    
    _MS(pthread_mutex_init(&log_thread->m_mutex, NULL));
    
    log_thread->m_cfg_active_request_count = 1;
    log_thread->m_cfg_net_buf_size = 2 * 1024 * 1024;
    _MS(log_thread->m_runing_thread = NULL);
    log_thread->m_watcher = NULL;
    log_thread->m_pipe_r_size = 0;
    log_thread->m_is_runing = 0;
    log_thread->m_net_schedule = NULL;
    log_thread->m_net_driver = NULL;
    log_thread->m_trans_mgr = NULL;
    log_thread->m_env_active = NULL;

    /*request*/
    log_thread->m_request_max_id = 0;
    log_thread->m_request_count = 0;
    log_thread->m_request_buf_size = 0;
    log_thread->m_active_request_count = 0;
    log_thread->m_cache_max_id = 0;
    log_thread->m_name = name;

    TAILQ_INIT(&log_thread->m_caches);
    TAILQ_INIT(&log_thread->m_waiting_requests);
    TAILQ_INIT(&log_thread->m_active_requests);
    TAILQ_INIT(&log_thread->m_free_requests);

    /*schedule*/
    log_thread->m_commit_last_error = net_log_thread_commit_error_none;
    log_thread->m_commit_last_error_count = 0;
    log_thread->m_commit_delay_until_ms = 0;
    log_thread->m_commit_delay_processor = NULL;

    /*pipe*/
    if (pipe(log_thread->m_pipe_fd) != 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: create pipe fail, error=%d (%s)!", name, errno, strerror(errno));
        mem_free(schedule->m_alloc, log_thread);
        return NULL;
    }

    if (cpe_sock_set_none_block(log_thread->m_pipe_fd[0], 1) != 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: set none block fail, error=%d (%s)!", name, errno, strerror(errno));
        close(log_thread->m_pipe_fd[0]);
        close(log_thread->m_pipe_fd[1]);
        mem_free(schedule->m_alloc, log_thread);
        return NULL;
    }

    if (log_thread->m_processor.m_init) {
        if (log_thread->m_processor.m_init(log_thread->m_processor.m_ctx, log_thread) != 0) {
            CPE_ERROR(schedule->m_em, "log: thread %s: processor init fail!", name);
            close(log_thread->m_pipe_fd[0]);
            close(log_thread->m_pipe_fd[1]);
            mem_free(schedule->m_alloc, log_thread);
            return NULL;
        }
    }

    mem_buffer_init(&log_thread->m_tmp_buffer, schedule->m_alloc);
    
    TAILQ_INSERT_TAIL(&schedule->m_threads, log_thread, m_next);
    return log_thread;
}

void net_log_thread_free(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);

    CPE_ERROR(schedule->m_em, "log: thread %s: free!", log_thread->m_name);
    
    assert(!log_thread->m_is_runing);
    assert(log_thread->m_watcher == NULL);
    assert(log_thread->m_net_schedule == NULL);
    assert(log_thread->m_net_driver == NULL);
    assert(log_thread->m_trans_mgr == NULL);
    assert(log_thread->m_commit_delay_processor == NULL);
    assert(TAILQ_EMPTY(&log_thread->m_waiting_requests));
    assert(TAILQ_EMPTY(&log_thread->m_active_requests));
    assert(TAILQ_EMPTY(&log_thread->m_free_requests));
    assert(TAILQ_EMPTY(&log_thread->m_caches));
    
    if (log_thread->m_processor.m_fini) {
        log_thread->m_processor.m_fini(log_thread->m_processor.m_ctx, log_thread);
    }
    
    close(log_thread->m_pipe_fd[0]);
    close(log_thread->m_pipe_fd[1]);
    log_thread->m_pipe_fd[0] = -1;
    log_thread->m_pipe_fd[1] = -1;

    _MS(pthread_mutex_destroy(&log_thread->m_mutex));
    
    TAILQ_REMOVE(&schedule->m_threads, log_thread, m_next);
    
    mem_free(schedule->m_alloc, log_thread);
}

int net_log_thread_send_cmd(net_log_thread_t log_thread, net_log_thread_cmd_t cmd, net_log_thread_t from_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    
    ASSERT_ON_THREAD(from_thread);
    
    _MS(pthread_mutex_lock(&log_thread->m_mutex));

    if (write(log_thread->m_pipe_fd[1], cmd, cmd->m_size) < 0) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: ==> (%s) %s | write thread fail, error=%d (%s)!",
            from_thread->m_name, log_thread->m_name, net_log_thread_cmd_dump(&from_thread->m_tmp_buffer, cmd), 
            errno, strerror(errno));
        _MS(pthread_mutex_unlock(&log_thread->m_mutex));
        return -1;
    }

    _MS(pthread_mutex_unlock(&log_thread->m_mutex));

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: ==> (%s) %s (size=%d)",
            from_thread->m_name, log_thread->m_name, net_log_thread_cmd_dump(&from_thread->m_tmp_buffer, cmd), cmd->m_size);
    }

    return 0;
}

int net_log_thread_update_env(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);
    assert(log_thread->m_is_runing);

    struct net_log_thread_cmd_update_env update_env_cmd;
    update_env_cmd.head.m_size = sizeof(update_env_cmd);
    update_env_cmd.head.m_cmd = net_log_thread_cmd_type_update_env;
    update_env_cmd.m_env = schedule->m_env_active;

    return net_log_thread_send_cmd(log_thread, (net_log_thread_cmd_t)&update_env_cmd, schedule->m_thread_main);
}

int net_log_thread_update_net(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);
    assert(log_thread->m_is_runing);
    
    struct net_log_thread_cmd_update_net update_net_cmd;
    update_net_cmd.head.m_size = sizeof(update_net_cmd);
    update_net_cmd.head.m_cmd = net_log_thread_cmd_type_update_net;
    update_net_cmd.m_local_ip_stack = net_schedule_local_ip_stack(schedule->m_net_schedule);
    
    return net_log_thread_send_cmd(log_thread, (net_log_thread_cmd_t)&update_net_cmd, schedule->m_thread_main);
}

const char * net_log_thread_cache_dir(net_log_thread_t log_thread, mem_buffer_t tmp_buffer) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    assert(schedule->m_cfg_cache_dir);
    
    mem_buffer_clear_data(tmp_buffer);

    if (mem_buffer_printf(tmp_buffer, "%s/%s", schedule->m_cfg_cache_dir, log_thread->m_name) < 0) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: format buffer fail", log_thread->m_name);
        return NULL;
    }

    return mem_buffer_make_continuous(tmp_buffer, 0);
}

const char * net_log_thread_cache_file(net_log_thread_t log_thread, uint32_t id, mem_buffer_t tmp_buffer) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    assert(schedule->m_cfg_cache_dir);
    
    mem_buffer_clear_data(tmp_buffer);

    if (mem_buffer_printf(tmp_buffer, "%s/%s/cache_%05d", schedule->m_cfg_cache_dir, log_thread->m_name, id) < 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: manage: format buffer fail", log_thread->m_name);
        return NULL;
    }

    return mem_buffer_make_continuous(tmp_buffer, 0);
}

void net_log_thread_set_state(net_log_thread_t log_thread, net_log_thread_state_t state) {
    ASSERT_ON_THREAD(log_thread);
    
    if (log_thread->m_state == state) return;
    
    net_log_schedule_t schedule = log_thread->m_schedule;
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: state %s ==> %s", log_thread->m_name,
            net_log_thread_state_str(log_thread->m_state), net_log_thread_state_str(state));
    }
    
    log_thread->m_state = state;
}

uint8_t net_log_thread_is_suspend(net_log_thread_t log_thread) {
    ASSERT_ON_THREAD(log_thread);
    if (log_thread->m_net_schedule == NULL) return 1;
    if (log_thread->m_env_active == NULL) return 1;
    if (net_schedule_local_ip_stack(log_thread->m_net_schedule) == net_local_ip_stack_none) return 1;
    if (log_thread->m_commit_delay_processor && net_timer_is_active(log_thread->m_commit_delay_processor)) return 1;
    
    return 0;
}

void net_log_thread_commit_schedule_delay(net_log_thread_t log_thread, net_log_thread_commit_error_t commit_error) {
    ASSERT_ON_THREAD(log_thread);

    assert(log_thread->m_commit_delay_processor);
    assert(commit_error != net_log_thread_commit_error_none);
    
    uint64_t baseRetryTime = 0;
    uint64_t maxRetryTime = 0;

    if (log_thread->m_commit_last_error != commit_error) {
        log_thread->m_commit_last_error = commit_error;
    }
    log_thread->m_commit_last_error_count++;

    switch(commit_error) {
    case net_log_thread_commit_error_none:
        assert(0);
        break;
    case net_log_thread_commit_error_network:
        maxRetryTime = 3600000;
        baseRetryTime = 1000;
        break;
    case net_log_thread_commit_error_quota_exceed:
        maxRetryTime = 60000;
        baseRetryTime = 3000;
    case net_log_thread_commit_error_package_time:
        maxRetryTime = 3000;
        baseRetryTime = 3000;
        break;
    }
    
    uint64_t sleepMs = baseRetryTime;
    uint8_t i;
    for(i = 1; i < log_thread->m_commit_last_error_count; ++i) {
        uint32_t nextSleepMs = sleepMs * 2;
        if (nextSleepMs < maxRetryTime) {
            sleepMs = nextSleepMs;
        }
        else {
            sleepMs = maxRetryTime;
            break;
        }
    }

    int64_t commit_delay_until_ms = cur_time_ms() + (int64_t)sleepMs;
    if (log_thread->m_commit_delay_until_ms == 0
        || commit_delay_until_ms > log_thread->m_commit_delay_until_ms)
    {
        log_thread->m_commit_delay_until_ms = commit_delay_until_ms;
        assert(log_thread->m_commit_delay_processor);
        net_timer_active(log_thread->m_commit_delay_processor, sleepMs);
    }
}

void net_log_thread_commit_delay(net_timer_t timer, void * ctx) {
    net_log_thread_t log_thread = ctx;
    ASSERT_ON_THREAD(log_thread);
    
    log_thread->m_commit_last_error = net_log_thread_commit_error_none;
    log_thread->m_commit_last_error_count = 0;
    log_thread->m_commit_delay_until_ms = 0;
    net_log_thread_check_active_requests(log_thread);
}

const char * net_log_thread_state_str(net_log_thread_state_t state) {
    switch(state) {
    case net_log_thread_state_runing:
        return "runing";
    case net_log_thread_state_stoping:
        return "stoping";
    case net_log_thread_state_stoped:
        return "stoped";
    }
}
