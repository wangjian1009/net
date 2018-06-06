#ifndef NET_DNS_SERVER_H_INCLEDED
#define NET_DNS_SERVER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_dns_server_t net_dns_server_create(net_dns_resolver_t resolver, net_address_t addr);
void net_dns_server_free(net_dns_server_t server);

NET_END_DECL

#endif
