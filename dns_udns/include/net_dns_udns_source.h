#ifndef NET_DNS_UDNS_SOURCE_H_INCLEDED
#define NET_DNS_UDNS_SOURCE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_udns_system.h"

NET_BEGIN_DECL

net_dns_udns_source_t
net_dns_udns_source_create(
    mem_allocrator_t alloc, error_monitor_t em, uint8_t debug,
    net_dns_manage_t manage, net_driver_t driver);

void net_dns_udns_source_free(net_dns_udns_source_t udns);

void net_dns_udns_source_reset(net_dns_udns_source_t udns);
int net_dns_udns_source_add_server(net_dns_udns_source_t udns, net_address_t address);
int net_dns_udns_source_start(net_dns_udns_source_t udns);

net_dns_udns_source_t net_dns_udns_source_from_source(net_dns_source_t source);

NET_END_DECL

#endif
