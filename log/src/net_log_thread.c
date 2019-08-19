#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_errno.h"
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
    
    mem_free(schedule->m_alloc, log_thread);
}

int net_log_thread_send_cmd(net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    _MS(pthread_mutex_lock(&log_thread->m_mutex));

    if (write(log_thread->m_pipe_fd[1], cmd, cmd->m_size) < 0) {
        CPE_ERROR(
            log_thread->m_schedule->m_em, "log: thread %s: write thread fail, error=%d (%s)!",
            log_thread->m_name, errno, strerror(errno));
        _MS(pthread_mutex_unlock(&log_thread->m_mutex));
        return -1;
    }

    _MS(pthread_mutex_unlock(&log_thread->m_mutex));
    
    mem_buffer_clear(&log_thread->m_tmp_buffer);
    
    return 0;
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
        CPE_ERROR(schedule->m_em, "log: %s: manage: format buffer fail", log_thread->m_name);
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
    
    switch(commit_error) {
    case net_log_thread_commit_error_none:
        break;
    case net_log_thread_commit_error_network:
    /*         if (request->m_last_send_error != net_log_request_send_network_error) { */
/*             request->m_last_send_error = net_log_request_send_network_error; */
/*             request->m_last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS; */
/*             request->m_first_error_time = (uint32_t)time(NULL); */
/*         } */
/*         else { */
/*             if (request->m_last_sleep_ms < MAX_NETWORK_ERROR_SLEEP_MS) { */
/*                 request->m_last_sleep_ms *= 2; */
/*             } */
/*             // only drop data when SEND_TIME_INVALID_FIX not defined */
/*             if (time(NULL) - request->m_first_error_time > DROP_FAIL_DATA_TIME_SECOND) { */
/*                 break; */
/*             } */
/*         } */
        break;
    case net_log_thread_commit_error_quota_exceed:
        break;
    }
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