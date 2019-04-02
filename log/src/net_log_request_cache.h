#ifndef NET_LOG_REQUEST_CACHE_H_INCLEDED
#define NET_LOG_REQUEST_CACHE_H_INCLEDED
#include "net_log_request_manage.h"

struct net_log_request_cache {
    net_log_request_manage_t m_mgr;
    TAILQ_ENTRY(net_log_request_cache) m_next;
};

net_log_request_cache_t net_log_request_cache_create(net_log_request_manage_t mgr);
void net_log_request_cache_free(net_log_request_cache_t cache);

int net_log_request_cache_push(net_log_request_cache_t cache, net_log_request_param_t send_param);

#endif
