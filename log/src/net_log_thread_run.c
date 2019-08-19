#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "net_schedule.h"
#include "net_watcher.h"
#include "net_trans_manage.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request.h"
#include "net_log_request_cache.h"

static void net_log_thread_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_log_thread_pipe_clear(net_log_thread_t thread);

static int net_log_thread_setup(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    uint8_t external_setup = 0;

    if (IS_ON_THREAD_MAIN(schedule)) {
        log_thread->m_net_schedule = schedule->m_net_schedule;
        log_thread->m_net_driver = schedule->m_net_driver;
    }
    else {
        log_thread->m_net_schedule = net_schedule_create(schedule->m_alloc, schedule->m_em, log_thread->m_cfg_net_buf_size);
        if (log_thread->m_net_schedule != NULL) {
            CPE_ERROR(schedule->m_em, "log: thread %s: setup: create net schedule fail", log_thread->m_name);
            goto SETUP_FAIL;
        }

        assert(log_thread->m_processor.m_setup);
        if (log_thread->m_processor.m_setup(log_thread->m_processor.m_ctx, log_thread, log_thread->m_net_schedule) != 0) {
            CPE_ERROR(schedule->m_em, "log: thread %s: setup: setup fail", log_thread->m_name);
            goto SETUP_FAIL;
        }
        external_setup = 1;
    }

    /* request->m_delay_process = net_timer_create(log_thread->m_net_driver, net_log_request_delay_commit, request); */
    /* if (request->m_delay_process == NULL) { */
    /*     CPE_ERROR( */
    /*         schedule->m_em, "log: %s: category [%d]%s: request %d: complete with result %s, create delay process timer fail", */
    /*         log_thread->m_name, category->m_id, category->m_name, request->m_id, */
    /*         net_log_request_send_result_str(send_result)); */

    /*     //TODO: net_log_category_add_fail_statistics(category, request->m_send_param->log_count); */
    /*     net_log_request_free(request); */
    /*     net_log_thread_check_active_requests(log_thread); */
    /*     return; */
    /* } */
    
    char trans_name[64];
    snprintf(trans_name, sizeof(trans_name), "log.%s", log_thread->m_name);
    log_thread->m_trans_mgr = 
        net_trans_manage_create(schedule->m_alloc, schedule->m_em, log_thread->m_net_schedule, log_thread->m_net_driver, trans_name);
    if (log_thread->m_trans_mgr == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: setup: create trans mgr fail", log_thread->m_name);
        goto SETUP_FAIL;
    }
    /* log_thread->m_request_mgr = NULL; */
    /*     net_log_request_manage_create( */
    /*         schedule, schedule->m_net_schedule, schedule->m_net_driver, */
    /*         schedule->m_cfg_active_request_count, "main", net_log_schedule_tmp_buffer(schedule), */
    /*         NULL, NULL); */
    /*     if (schedule->m_main_thread_request_mgr == NULL) { */
    /*         CPE_ERROR(schedule->m_em, "log: schedule: create main thread request mgr fail"); */
    /*         return -1; */
    /*     } */

    /*     if (schedule->m_cfg_cache_dir) { /\*加载缓存 *\/ */
    /*         if (net_log_request_mgr_init_cache_dir(schedule->m_main_thread_request_mgr) != 0) return -1; */
    /*         if (net_log_request_mgr_search_cache(schedule->m_main_thread_request_mgr) != 0) return -1; */
    /*         net_log_request_mgr_check_active_requests(schedule->m_main_thread_request_mgr); */
    /*     } */

    /* if (thread->m_bind_watcher != NULL) { */
    /*     CPE_ERROR(schedule->m_em, "log: thread %s: bind: already binded!", thread->m_name); */
    /*     return -1; */
    /* } */

    log_thread->m_watcher = net_watcher_create(log_thread->m_net_driver, log_thread->m_pipe_fd[0], log_thread, net_log_thread_rw_cb);
    if (log_thread->m_watcher == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: setup: create watcher fail!", log_thread->m_name);
        goto SETUP_FAIL;
    }
    net_watcher_update(log_thread->m_watcher, 1, 0);
    
    return 0;
    
SETUP_FAIL:
    if (log_thread->m_watcher) {
        net_watcher_free(log_thread->m_watcher);
        log_thread->m_watcher = NULL;
    }
    
    if (log_thread->m_trans_mgr) {
        net_trans_manage_free(log_thread->m_trans_mgr);
        log_thread->m_trans_mgr = NULL;
    }

    if (external_setup) {
        if (log_thread->m_processor.m_teardown) {
            log_thread->m_processor.m_teardown(log_thread->m_processor.m_ctx, log_thread, log_thread->m_net_schedule);
        }
    }
    log_thread->m_net_driver = NULL;
        
    if (log_thread->m_schedule) {
        if (!IS_ON_THREAD_MAIN(schedule)) {
            net_log_schedule_free(log_thread->m_schedule);
        }
        log_thread->m_net_schedule = NULL;
    }
    
    return -1;
}

static void net_log_thread_teardown(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;

    while(!TAILQ_EMPTY(&log_thread->m_waiting_requests)) {
        net_log_request_free(TAILQ_FIRST(&log_thread->m_waiting_requests));
    }

    while(!TAILQ_EMPTY(&log_thread->m_active_requests)) {
        net_log_request_free(TAILQ_FIRST(&log_thread->m_active_requests));
    }
    
    while(!TAILQ_EMPTY(&log_thread->m_free_requests)) {
        net_log_request_real_free(TAILQ_FIRST(&log_thread->m_free_requests));
    }

    while(!TAILQ_EMPTY(&log_thread->m_caches)) {
        net_log_request_cache_free(TAILQ_FIRST(&log_thread->m_caches));
    }

    if (log_thread->m_watcher) {
        net_watcher_free(log_thread->m_watcher);
        log_thread->m_watcher = NULL;
    }

    if (log_thread->m_trans_mgr) {
        net_trans_manage_free(log_thread->m_trans_mgr);
        log_thread->m_trans_mgr = NULL;
    }
    
    if (schedule->m_cfg_cache_dir) { /*保存缓存 */
        net_log_thread_save_and_clear_requests(log_thread);
    }

    if (!IS_ON_THREAD_MAIN(schedule)) {
        if (log_thread->m_processor.m_teardown) {
            log_thread->m_processor.m_teardown(log_thread->m_processor.m_ctx, log_thread, log_thread->m_net_schedule);
        }
        
        if (log_thread->m_net_schedule) {
            net_schedule_free(log_thread->m_net_schedule);
        }
    }
    log_thread->m_net_schedule = NULL;
    log_thread->m_net_driver = NULL;
}

#if NET_LOG_MULTI_THREAD
static void * net_log_thread_execute(void * param) {
    net_log_thread_t log_thread = param;
    net_log_schedule_t schedule = log_thread->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: thread %s: thread: thread starated", log_thread->m_name);
    }

    assert(log_thread->m_watcher == NULL);
    assert(log_thread->m_net_schedule == NULL);
    assert(log_thread->m_net_driver == NULL);
    assert(log_thread->m_trans_mgr == NULL);
    assert(TAILQ_EMPTY(&log_thread->m_waiting_requests));
    assert(TAILQ_EMPTY(&log_thread->m_active_requests));
    assert(TAILQ_EMPTY(&log_thread->m_free_requests));
    assert(TAILQ_EMPTY(&log_thread->m_caches));

    if (net_log_thread_setup(log_thread) != 0) {
        return NULL;
    }

    assert(log_thread->m_processor.m_run);
    log_thread->m_processor.m_run(log_thread->m_processor.m_ctx, log_thread);

    struct net_log_thread_cmd_stoped stop_cmd;
    stop_cmd.head.m_size = sizeof(stop_cmd);
    stop_cmd.head.m_cmd = net_log_thread_cmd_type_stoped;
    stop_cmd.log_thread = log_thread;
    assert(schedule->m_thread_main);
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&stop_cmd);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: thread %s: thread: thread stoped", log_thread->m_name);
    }

    return NULL;
}
#endif

int net_log_thread_start(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);

#if NET_LOG_MULTI_THREAD
    if (log_thread->m_runing_thread != NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: start: already started", log_thread->m_name);
        return -1;
    }
    
    log_thread->m_runing_thread = mem_alloc(schedule->m_alloc, sizeof(pthread_t));
    if (log_thread->m_runing_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: start: alloc pthread_t fail", log_thread->m_name);
        return -1;
    }

    if (pthread_create(log_thread->m_runing_thread, NULL, net_log_thread_execute, log_thread) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: start: pthread_create fail, error=%d (%s)!", 
            log_thread->m_name, errno, strerror(errno));
        mem_free(schedule->m_alloc, log_thread->m_runing_thread);
        log_thread->m_runing_thread = NULL;
        return -1;
    }
    
#else   
    if (net_log_thread_setup(log_thread) != 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: start: thread setup fail!", log_thread->m_name);
        return -1;
    }
#endif

    return 0;
}

void net_log_thread_notify_stop_force(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);
    
    struct net_log_thread_cmd stop_cmd;
    stop_cmd.m_size = sizeof(stop_cmd);
    stop_cmd.m_cmd = net_log_thread_cmd_type_stop_force;
    net_log_thread_send_cmd(log_thread, &stop_cmd);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: thread %s: notify stop complete: notify success", log_thread->m_name);
    }
}

void net_log_thread_wait_stop(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);

#if NET_LOG_MULTI_THREAD
    if (log_thread->m_runing_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: wait stop: thread already stoped", log_thread->m_name);
        return;
    }

    if (pthread_join(*log_thread->m_runing_thread, NULL) != 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: wait stop: wait error, errno=%d (%s)", log_thread->m_name, errno, strerror(errno));
    }

    mem_free(schedule->m_alloc, log_thread->m_runing_thread);
    log_thread->m_runing_thread = NULL;

#else
    net_log_thread_pipe_clear(log_thread);
    net_log_thread_teardown(log_thread);
#endif
    
    assert(schedule->m_runing_thread_count > 0);
    schedule->m_runing_thread_count--;
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: wait stop: wait success, thread-count=%d",
            log_thread->m_name, schedule->m_runing_thread_count);
    }

    net_log_schedule_check_stop_complete(schedule);
}

static void net_log_thread_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_log_thread_t log_thread = ctx;
    net_log_schedule_t schedule = log_thread->m_schedule;

    if (do_read) {
        uint8_t need_process = 1;

        while(need_process) {
            ssize_t rv = read(
                log_thread->m_pipe_fd[0], log_thread->m_pipe_r_buf + log_thread->m_pipe_r_size,
                sizeof(log_thread->m_pipe_r_buf) - log_thread->m_pipe_r_size);
            if (rv < 0) {
                if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                }
                else {
                    CPE_ERROR(
                        schedule->m_em, "log: thread %s: read thread fail, error=%d (%s)!",
                        log_thread->m_name, errno, strerror(errno));
                }
                need_process = 0;
            }
            else {
                log_thread->m_pipe_r_size += (uint16_t)rv;
                assert(log_thread->m_pipe_r_size <= sizeof(log_thread->m_pipe_r_buf));
            }

            while(log_thread->m_pipe_r_size >= sizeof(struct net_log_thread_cmd)) {
                net_log_thread_cmd_t cmd = (net_log_thread_cmd_t)log_thread->m_pipe_r_buf;

                if (log_thread->m_pipe_r_size < cmd->m_size) {
                    need_process = 0;
                    break;
                }

                net_log_thread_dispatch(log_thread, cmd);

                memmove(log_thread->m_pipe_r_buf, log_thread->m_pipe_r_buf + cmd->m_size, log_thread->m_pipe_r_size - cmd->m_size);
                log_thread->m_pipe_r_size -= cmd->m_size;
            }
        }
    }
}

static void net_log_thread_pipe_clear(net_log_thread_t log_thread) {
}
