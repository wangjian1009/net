#ifndef NET_DNS_SOURCE_NAMESERVER_H_INCLEDED
#define NET_DNS_SOURCE_NAMESERVER_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_source_nameserver_t net_dns_source_nameserver_create(net_dns_manage_t manage, net_address_t addr, uint8_t is_own);
void net_dns_source_nameserver_free(net_dns_source_nameserver_t server);

net_address_t net_dns_source_nameserver_address(net_dns_source_nameserver_t server);
    
NET_END_DECL

#endif
