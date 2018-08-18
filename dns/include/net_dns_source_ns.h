#ifndef NET_DNS_SOURCE_NS_H_INCLEDED
#define NET_DNS_SOURCE_NS_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_source_ns_t
net_dns_source_ns_create(
    net_dns_manage_t manage, net_driver_t driver, net_address_t addr, uint8_t is_own);
void net_dns_source_ns_free(net_dns_source_ns_t server);

net_dns_source_ns_t net_dns_source_ns_find(net_dns_manage_t manage, net_address_t addr);

net_address_t net_dns_source_ns_address(net_dns_source_ns_t server);
    
NET_END_DECL

#endif
