#ifndef NET_DNS_RESOLVER_H_INCLEDED
#define NET_DNS_RESOLVER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_dns_resolver_t net_dns_resolver_create(net_schedule_t schedule, net_dns_mode_t mode);
void net_dns_resolver_free(net_dns_resolver_t resolver);

net_dns_resolver_t net_dns_resolver_get(net_schedule_t schedule);

net_dns_mode_t net_dns_resolver_mode(net_dns_resolver_t resolver);

NET_END_DECL

#endif
