#include "net_address.h"
#include "net_dns_svr_monitor_i.h"

net_dns_svr_monitor_t
net_dns_svr_monitor_create(net_dns_svr_t dns_svr, net_dns_svr_monitor_fun_t fun, void * ctx) {
    net_dns_svr_monitor_t monitor = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_monitor));
    if (monitor == NULL) {
        CPE_ERROR(dns_svr->m_em, "dns-svr: monitor alloc fail!");
        return NULL;
    }

    monitor->m_svr = dns_svr;
    monitor->m_fun = fun;
    monitor->m_ctx = ctx;

    TAILQ_INSERT_TAIL(&dns_svr->m_monitors, monitor, m_next);
    return monitor;
}

void net_dns_svr_monitor_free(net_dns_svr_monitor_t monitor) {
    net_dns_svr_t dns_svr = monitor->m_svr;
    TAILQ_REMOVE(&dns_svr->m_monitors, monitor, m_next);
    mem_free(dns_svr->m_alloc, monitor);
}

void net_dns_svr_notify_query_result(net_dns_svr_t svr, net_dns_svr_query_error_t error, net_dns_svr_query_t query) {
    net_dns_svr_monitor_t monitor;

    TAILQ_FOREACH(monitor, &svr->m_monitors, m_next) {
        monitor->m_fun(monitor->m_ctx, error, query);
    }
}
