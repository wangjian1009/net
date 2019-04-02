#ifndef NET_LOG_REQUEST_MANAGE_H_INCLEDED
#define NET_LOG_REQUEST_MANAGE_H_INCLEDED
#include "curl/curl.h"
#include "net_log_schedule_i.h"

typedef enum net_log_request_manage_state {
    net_log_request_manage_state_runing,
    net_log_request_manage_state_pause,
} net_log_request_manage_state_t;

struct net_log_request_manage {
    net_log_schedule_t m_schedule;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    void (*m_stop_fun)(void * ctx);
    void * m_stop_ctx;
    net_log_request_manage_state_t m_state;
    uint8_t m_cfg_active_request_count;
    uint8_t m_active_request_count;
    uint16_t m_request_count;
    uint32_t m_request_max_id;
    net_log_request_cache_t m_cache;
    net_log_request_list_t m_waiting_requests;
    net_log_request_list_t m_active_requests;
    net_log_request_list_t m_free_requests;
    net_timer_t m_timer_event;
	CURLM * m_multi_handle;
    int m_still_running;
    const char * m_name;
    mem_buffer_t m_tmp_buffer;
};

net_log_request_manage_t
net_log_request_manage_create(
    net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver,
    uint8_t max_active_request_count, const char * name, mem_buffer_t tmp_buffer,
    void (*stop_fun)(void * ctx), void * stop_ctx);

void net_log_request_manage_free(net_log_request_manage_t request_mgr);

void net_log_request_manage_process_cmd(
    net_log_request_manage_t mgr, net_log_request_cmd_t cmd, net_log_request_param_t send_param);

void net_log_request_manage_active_next(net_log_request_manage_t mgr);

int net_log_request_manage_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);
int net_log_request_manage_timer_cb(CURLM *multi, long timeout_ms, net_log_request_manage_t mgr);
void net_log_request_manage_do_timeout(net_timer_t timer, void * ctx);

#endif
