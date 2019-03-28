#ifndef NET_LOG_REQUEST_H_INCLEDED
#define NET_LOG_REQUEST_H_INCLEDED
#include "curl/curl.h"
#include "net_log_request_manage.h"

struct net_log_request {
    net_log_request_manage_t m_mgr;
    TAILQ_ENTRY(net_log_request) m_next;
    CURL * m_handler;
};

net_log_request_t net_log_request_create(net_log_request_manage_t mgr, log_producer_send_param_t send_param);
void net_log_request_free(net_log_request_t request);

void net_log_request_real_free(net_log_request_t request);

#endif
