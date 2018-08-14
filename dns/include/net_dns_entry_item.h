#ifndef NET_DNS_ENTRY_ITEM_H_INCLEDED
#define NET_DNS_ENTRY_ITEM_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_entry_item_it {
    net_dns_entry_item_t (*next)(net_dns_entry_item_it_t it);
    char data[64];
};

net_dns_entry_item_t
net_dns_entry_item_create(
    net_dns_entry_t entry, net_dns_source_t source,
    net_address_t address, uint8_t is_own,
    int64_t expire_time_ms);

void net_dns_entry_item_free(net_dns_entry_item_t item);

net_dns_entry_t net_dns_entry_item_entry(net_dns_entry_item_t item);
net_dns_source_t net_dns_entry_item_source(net_dns_entry_item_t item);
net_address_t net_dns_entry_item_address(net_dns_entry_item_t item);

#define net_dns_entry_item_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
