#ifndef NET_DNS_ENTRY_H_INCLEDED
#define NET_DNS_ENTRY_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_entry_it {
    net_dns_entry_t (*next)(net_dns_entry_it_t it);
    char data[64];
};

net_dns_entry_t
net_dns_entry_create(net_dns_manage_t manage, net_address_t hostname);

void net_dns_entry_free(net_dns_entry_t entry);

net_dns_entry_t net_dns_entry_find(net_dns_manage_t manage, net_address_t hostname);

net_address_t net_dns_entry_hostname(net_dns_entry_t entry);
const char * net_dns_entry_hostname_str(net_dns_entry_t entry);

uint8_t net_dns_entry_is_origin_of(net_dns_entry_t entry, net_dns_entry_t check_start);

void net_dns_entry_clear_item_by_source(net_dns_entry_t entry, net_dns_source_t source);

net_dns_entry_item_t net_dns_entry_select_item(net_dns_entry_t entry, net_dns_item_select_policy_t policy, net_dns_query_type_t query_type);

void net_dns_entry_addresses(net_dns_entry_t entry, net_address_it_t it, uint8_t recursive);
void net_dns_entry_items(net_dns_entry_t entry, net_dns_entry_item_it_t it, uint8_t recursive);
void net_dns_entry_cnames(net_dns_entry_t entry, net_dns_entry_it_t it);

#define net_dns_entry_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
