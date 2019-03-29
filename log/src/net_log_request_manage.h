#ifndef NET_LOG_REQUEST_MANAGE_H_INCLEDED
#define NET_LOG_REQUEST_MANAGE_H_INCLEDED
#include "curl/curl.h"
#include "net_log_schedule_i.h"

struct net_log_request_manage {
    net_log_schedule_t m_schedule;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    uint32_t m_request_max_id;
    net_log_request_list_t m_requests;
    net_log_request_list_t m_free_requests;
    net_timer_t m_timer_event;
	CURLM * m_multi_handle;
    int m_still_running;
};

net_log_request_manage_t net_log_request_manage_create(net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver);
void net_log_request_manage_free(net_log_request_manage_t request_mgr);

int net_log_request_manage_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);
int net_log_request_manage_timer_cb(CURLM *multi, long timeout_ms, net_log_request_manage_t mgr);
void net_log_request_manage_do_timeout(net_timer_t timer, void * ctx);

#endif
