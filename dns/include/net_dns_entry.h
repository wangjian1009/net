#ifndef NET_DNS_ENTRY_H_INCLEDED
#define NET_DNS_ENTRY_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_entry_t
net_dns_entry_create(
    net_dns_manage_t manage, const char * hostname,
    net_address_t address, uint8_t is_own,
    uint32_t expire_time_ms);

void net_dns_entry_free(net_dns_manage_t manage, net_dns_entry_t entry);

net_dns_entry_t net_dns_entry_find(net_dns_manage_t manage, const char * hostname);

NET_END_DECL

#endif
