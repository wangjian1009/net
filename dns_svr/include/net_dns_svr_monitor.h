#ifndef NET_DNS_SVR_MONITOR_H_INCLEDED
#define NET_DNS_SVR_MONITOR_H_INCLEDED
#include "net_dns_svr_types.h"

NET_BEGIN_DECL

typedef void (*net_dns_svr_monitor_fun_t)(void * ctx, net_dns_svr_query_error_t error, net_dns_svr_query_t query);

net_dns_svr_monitor_t net_dns_svr_monitor_create(net_dns_svr_t dns_svr, net_dns_svr_monitor_fun_t fun, void * ctx);
void net_dns_svr_monitor_free(net_dns_svr_monitor_t monitor);

NET_END_DECL

#endif
