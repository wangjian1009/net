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

typedef enum net_log_thread_state {
    net_log_thread_state_runing,
    net_log_thread_state_pause,
    net_log_thread_state_stoping,
    net_log_thread_state_stoped,
} net_log_thread_state_t;

struct net_log_thread {
    net_log_schedule_t m_schedule;
    error_monitor_t m_em;
    TAILQ_ENTRY(net_log_thread) m_next;
    const char * m_name;
    uint16_t m_cfg_active_request_count;
    uint32_t m_cfg_net_buf_size;
    net_log_thread_state_t m_state;

    /*env*/
    net_log_env_t m_env_active;
    
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

    /*net*/
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    net_trans_manage_t m_trans_mgr;
    
    /*requests*/
    uint16_t m_active_request_count;
    uint16_t m_request_count;
    uint32_t m_request_buf_size;
    uint32_t m_request_max_id;
    uint32_t m_cache_max_id;
    net_log_request_cache_list_t m_caches;
    net_log_request_list_t m_waiting_requests;
    net_log_request_list_t m_active_requests;
    net_log_request_list_t m_free_requests;
    
    /*tmp_buffer*/
    struct mem_buffer m_tmp_buffer;
};

void net_log_thread_free(net_log_thread_t log_thread);

void net_log_thread_set_state(net_log_thread_t log_thread, net_log_thread_state_t state);

int net_log_thread_start(net_log_thread_t log_thread);
void net_log_thread_notify_stop_force(net_log_thread_t log_thread);
void net_log_thread_wait_stop(net_log_thread_t log_thread);

int net_log_thread_send_cmd(net_log_thread_t log_thread, net_log_thread_cmd_t cmd);
void net_log_thread_dispatch(net_log_thread_t log_thread, net_log_thread_cmd_t cmd);

/*cache*/
const char * net_log_thread_cache_dir(net_log_thread_t log_thread, mem_buffer_t tmp_buffer);
const char * net_log_thread_cache_file(net_log_thread_t log_thread, uint32_t id, mem_buffer_t tmp_buffer);
int net_log_thread_search_cache(net_log_thread_t log_thread);
int net_log_thread_save_and_clear_requests(net_log_thread_t log_thread);

/*request*/
uint8_t net_log_thread_request_is_empty(net_log_thread_t log_thread);
void net_log_thread_check_active_requests(net_log_thread_t log_thread);

/*process*/
void net_log_thread_process_cmd_send(net_log_thread_t log_thread, net_log_request_param_t send_param);
void net_log_thread_process_cmd_pause(net_log_thread_t log_thread);
void net_log_thread_process_cmd_resume(net_log_thread_t log_thread);
void net_log_thread_process_cmd_stop_begin(net_log_thread_t log_thread);
void net_log_thread_process_cmd_stop_force(net_log_thread_t log_thread);

/*strings*/
const char * net_log_thread_state_str(net_log_thread_state_t state);

#endif
