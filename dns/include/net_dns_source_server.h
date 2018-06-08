#ifndef NET_DNS_SOURCE_SERVER_H_INCLEDED
#define NET_DNS_SOURCE_SERVER_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_source_server_t net_dns_source_server_create(net_dns_manage_t manage, net_address_t addr, uint8_t is_own);
void net_dns_source_server_free(net_dns_source_server_t server);

net_address_t net_dns_source_server_address(net_dns_source_server_t server);
    
NET_END_DECL

#endif
