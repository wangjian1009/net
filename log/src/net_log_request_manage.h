#ifndef NET_LOG_REQUEST_MANAGE_H_INCLEDED
#define NET_LOG_REQUEST_MANAGE_H_INCLEDED
#include "net_log_schedule_i.h"

typedef enum net_log_request_manage_state {
    net_log_request_manage_state_runing,
    net_log_request_manage_state_pause,
    net_log_request_manage_state_stoping,
    net_log_request_manage_state_stoped,
} net_log_request_manage_state_t;

struct net_log_request_manage {
    net_log_schedule_t m_schedule;
    net_log_env_t m_env_active;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    net_trans_manage_t m_trans_mgr;
    void (*m_stop_fun)(void * ctx);
    void * m_stop_ctx;
    net_log_request_manage_state_t m_state;
    uint16_t m_cfg_active_request_count;
    uint16_t m_active_request_count;
    uint16_t m_request_count;
    uint32_t m_request_buf_size;
    uint32_t m_request_max_id;
    uint32_t m_cache_max_id;
    net_log_request_cache_list_t m_caches;
    net_log_request_list_t m_waiting_requests;
    net_log_request_list_t m_active_requests;
    net_log_request_list_t m_free_requests;
    const char * m_name;
    mem_buffer_t m_tmp_buffer;
};

net_log_request_manage_t
net_log_request_manage_create(
    net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver,
    uint16_t max_active_request_count, const char * name, mem_buffer_t tmp_buffer,
    void (*stop_fun)(void * ctx), void * stop_ctx);

void net_log_request_manage_free(net_log_request_manage_t request_mgr);

void net_log_request_manage_process_cmd_send(net_log_request_manage_t mgr, net_log_request_param_t send_param);
void net_log_request_manage_process_cmd_pause(net_log_request_manage_t mgr);
void net_log_request_manage_process_cmd_resume(net_log_request_manage_t mgr);
void net_log_request_manage_process_cmd_stop_begin(net_log_request_manage_t mgr);
void net_log_request_manage_process_cmd_stop_complete(net_log_request_manage_t mgr);

const char * net_log_request_manage_cache_dir(net_log_request_manage_t mgr, mem_buffer_t tmp_buffer);
const char * net_log_request_manage_cache_file(net_log_request_manage_t mgr, uint32_t id, mem_buffer_t tmp_buffer);

int net_log_request_mgr_search_cache(net_log_request_manage_t mgr);
void net_log_request_mgr_check_active_requests(net_log_request_manage_t mgr);

int net_log_request_mgr_save_and_clear_requests(net_log_request_manage_t mgr);

int net_log_request_mgr_init_cache_dir(net_log_request_manage_t mgr);

uint8_t net_log_request_mgr_is_empty(net_log_request_manage_t mgr);

const char * net_log_request_manage_state_str(net_log_request_manage_state_t state);

#endif
