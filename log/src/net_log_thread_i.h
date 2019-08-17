#ifndef NET_LOG_THREAD_I_H_INCLEDED
#define NET_LOG_THREAD_I_H_INCLEDED
#include "net_log_thread.h"
#include "net_log_schedule_i.h"

#define ASSERT_ON_THREAD(__thread) _MS(assert((__thread)->m_runing_thread && (*(__thread)->m_runing_thread) == pthread_self()))
#define ASSERT_ON_THREAD_MAIN(__schedule) _MS(assert((__schedule)->m_main_thread_id == pthread_self()))

#if NET_LOG_MULTI_THREAD
#define IS_ON_THREAD(__thread) ((__thread)->m_runing_thread && (*(__thread)->m_runing_thread) == pthread_self())
#define IS_ON_THREAD_MAIN(__schedule) ((__schedule)->m_main_thread_id == pthread_self())
#else
#define IS_ON_THREAD(__thread) (1 == 1)
#define IS_ON_THREAD_MAIN(__thread) (1 == 1)
#endif


struct net_log_thread {
    net_log_schedule_t m_schedule;
    error_monitor_t m_em;
    TAILQ_ENTRY(net_log_thread) m_next;
    const char * m_name;
    uint16_t m_cfg_active_request_count;
    uint32_t m_cfg_net_buf_size;
    net_log_request_manage_t m_request_mgr;

    /*thread op */
    struct net_log_thread_processor m_processor;

    /*thread*/
    _MS(pthread_t * m_runing_thread);
    uint8_t m_is_runing;

    /*pipe*/
    _MS(pthread_mutex_t m_mutex);
    int m_pipe_fd[2];
    char m_pipe_r_buf[64];
    uint16_t m_pipe_r_size;
    net_watcher_t m_watcher;

    /**/
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    
    /**/
    struct mem_buffer m_tmp_buffer;
};

void net_log_thread_free(net_log_thread_t thread);

int net_log_thread_start(net_log_thread_t thread);
void net_log_thread_notify_stop_force(net_log_thread_t thread);
void net_log_thread_wait_stop(net_log_thread_t thread);

int net_log_thread_send_cmd(net_log_thread_t thread, net_log_thread_cmd_t cmd);
void net_log_thread_dispatch(net_log_thread_t thread, net_log_thread_cmd_t cmd);

#endif
