#ifndef NET_DNS_UDNS_SOURCE_H_INCLEDED
#define NET_DNS_UDNS_SOURCE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_udns_system.h"

NET_BEGIN_DECL

net_dns_udns_source_t
net_dns_udns_source_create(
    mem_allocrator_t alloc, error_monitor_t em,
    net_dns_manage_t manage, net_driver_t driver);

void net_dns_udns_source_free(net_dns_udns_source_t udns);

net_dns_udns_source_t net_dns_udns_source_from_source(net_dns_source_t source);

NET_END_DECL

#endif
