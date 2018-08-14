#ifndef NET_DNS_ENTRY_H_INCLEDED
#define NET_DNS_ENTRY_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_entry_t
net_dns_entry_create(net_dns_manage_t manage, const char * hostname);

void net_dns_entry_free(net_dns_entry_t entry);

net_dns_entry_t net_dns_entry_find(net_dns_manage_t manage, const char * hostname);

net_dns_task_t net_dns_entry_task(net_dns_entry_t entry);
const char * net_dns_entry_hostname(net_dns_entry_t entry);

net_dns_entry_t net_dns_entry_main(net_dns_entry_t entry);
int net_dns_entry_set_main(net_dns_entry_t entry, net_dns_entry_t main);
uint8_t net_dns_entry_is_main_of(net_dns_entry_t entry, net_dns_entry_t check_start);

net_dns_entry_t net_dns_entry_effective(net_dns_entry_t entry);

void net_dns_entry_clear_item_by_source(net_dns_entry_t entry, net_dns_source_t source);

net_dns_entry_item_t net_dns_entry_select_item(net_dns_entry_t entry, net_dns_item_select_policy_t policy);

void net_dns_entry_addresses(net_dns_entry_t entry, net_address_it_t it);
void net_dns_entry_items(net_dns_entry_t entry, net_dns_entry_item_it_t it);

NET_END_DECL

#endif
