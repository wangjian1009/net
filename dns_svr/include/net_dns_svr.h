#ifndef NET_DNS_SVR_H_INCLEDED
#define NET_DNS_SVR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_svr_types.h"

NET_BEGIN_DECL

net_dns_svr_t net_dns_svr_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule);
void net_dns_svr_free(net_dns_svr_t dns_svr);

uint8_t net_dns_svr_debug(net_dns_svr_t dns_svr);
void net_dns_svr_set_debug(net_dns_svr_t dns_svr, uint8_t debug);

net_schedule_t net_dns_svr_schedule(net_dns_svr_t dns_svr);

NET_END_DECL

#endif
