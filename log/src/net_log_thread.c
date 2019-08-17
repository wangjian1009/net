#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "net_schedule.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request_manage.h"

static void net_log_thread_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_log_thread_pipe_clear(net_log_schedule_t schedule);

net_log_thread_t net_log_thread_create(net_log_schedule_t schedule, const char * name, net_log_thread_processor_t processor) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    net_log_thread_t log_thread = mem_alloc(schedule->m_alloc, sizeof(struct net_log_thread) + (processor ? processor->m_capacity : 0));

    log_thread->m_schedule = schedule;
    log_thread->m_em = schedule->m_em;
    log_thread->m_name = name;

    if (processor) {
        log_thread->m_processor = *processor;
    }
    else {
        bzero(&log_thread->m_processor, sizeof(log_thread->m_processor));
    }
    
    _MS(pthread_mutex_init(&log_thread->m_mutex, NULL));
    
    log_thread->m_cfg_active_request_count = 1;
    log_thread->m_cfg_net_buf_size = 2 * 1024 * 1024;
    log_thread->m_runing_thread = NULL;
    log_thread->m_watcher = NULL;
    log_thread->m_request_mgr = NULL;
    log_thread->m_pipe_r_size = 0;
    log_thread->m_is_runing = 0;
    log_thread->m_net_schedule = NULL;
    log_thread->m_net_driver = NULL;

    
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
    assert(log_thread->m_request_mgr == NULL);
    assert(log_thread->m_net_schedule == NULL);
    assert(log_thread->m_net_driver == NULL);
    
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

static int net_log_thread_setup(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    if (IS_ON_THREAD_MAIN(schedule)) {
        log_thread->m_net_schedule = schedule->m_net_schedule;
        log_thread->m_net_driver = schedule->m_net_driver;
    }
    else {
        log_thread->m_net_schedule = net_schedule_create(schedule->m_alloc, log_thread->m_em, log_thread->m_cfg_net_buf_size);
        if (log_thread->m_net_schedule != NULL) {
            CPE_ERROR(schedule->m_em, "log: thread %s: setup: create net schedule fail", log_thread->m_name);
            return -1;
        }
        
        assert(log_thread->m_processor.m_setup);
        if (log_thread->m_processor.m_setup(log_thread->m_processor.m_ctx, log_thread, log_thread->m_net_schedule) != 0) {
            CPE_ERROR(schedule->m_em, "log: thread %s: setup: setup fail", log_thread->m_name);
            return -1;
        }
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

    /* thread->m_bind_watcher = net_watcher_create(net_driver, thread->m_thread_fd[0], pipe, net_log_pipe_action); */
    /* if (pipe->m_bind_watcher == NULL) { */
    /*     CPE_ERROR(schedule->m_em, "log: pipe %s: bind: create watcher fail!", pipe->m_name); */
    /*     return -1; */
    /* } */

    /* net_watcher_update(pipe->m_bind_watcher, 1, 0); */
    
    return 0;
}

static void net_log_thread_teardown(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;

    if (log_thread->m_request_mgr) {
        if (schedule->m_cfg_cache_dir) { /*保存缓存 */
            net_log_request_mgr_save_and_clear_requests(log_thread->m_request_mgr);
        }
        net_log_request_manage_free(log_thread->m_request_mgr);
        log_thread->m_request_mgr = NULL;
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

    assert(log_thread->m_runing_thread == NULL);
    assert(log_thread->m_watcher == NULL);
    assert(log_thread->m_request_mgr == NULL);
    assert(log_thread->m_net_schedule == NULL);
    assert(log_thread->m_net_driver == NULL);

    if (net_log_thread_setup(log_thread) != 0) {
        return NULL;
    }

    assert(log_thread->m_processor.m_run);
    log_thread->m_processor.m_run(log_thread->m_processor.m_ctx, log_thread);

    struct net_log_thread_cmd_stoped stop_cmd;
    stop_cmd.head.m_size = sizeof(stop_cmd);
    stop_cmd.head.m_cmd = net_log_thread_cmd_stoped;
    stop_cmd.log_thread = log_thread;
    assert(schedule->m_thread_main);
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&stop_cmd);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: thread: thread stoped", log_thread->m_name);
    }

    return NULL;
}
#endif

int net_log_thread_start(net_log_thread_t thread) {
    net_log_schedule_t schedule = thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);

#if NET_LOG_MULTI_THREAD
    /* assert(thread->m_runing_thread == NULL); */
    /* thread->m_runing_thread = mem_alloc(schedule->m_alloc, sizeof(pthread_t)); */
    /* if (thread->m_runing_thread == NULL) { */
    /*     CPE_ERROR(schedule->m_em, "log: thread %s: setup: alloc pthread_t fail", thread->m_name); */
    /*     return -1; */
    /* } */
    /* *thread->m_runing_thread = pthread_self(); */
    
#else    
#endif

    return 0;
}

void net_log_thread_notify_stop_force(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD_MAIN(schedule);
    
    if (log_thread->m_runing_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: thread %s: notify stop complete: thread already stoped", log_thread->m_name);
        return;
    }

    struct net_log_thread_cmd stop_cmd;
    stop_cmd.m_size = sizeof(stop_cmd);
    stop_cmd.m_cmd = net_log_thread_cmd_stop_force;
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
    net_log_thread_pipe_clear(thread);
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

static void net_log_thread_pipe_clear(net_log_schedule_t schedule) {
}
