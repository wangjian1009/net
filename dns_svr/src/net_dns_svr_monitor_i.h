#ifndef NET_DNS_SVR_MONITOR_I_H_INCLEDED
#define NET_DNS_SVR_MONITOR_I_H_INCLEDED
#include "net_dns_svr_monitor.h"
#include "net_dns_svr_i.h"

NET_BEGIN_DECL

struct net_dns_svr_monitor {
    net_dns_svr_t m_svr;
    TAILQ_ENTRY(net_dns_svr_monitor) m_next;
    net_dns_svr_monitor_fun_t m_fun;
    void * m_ctx;
};

void net_dns_svr_notify_query_result(net_dns_svr_t svr, net_dns_svr_query_error_t error, net_dns_svr_query_t query);

NET_END_DECL

#endif


