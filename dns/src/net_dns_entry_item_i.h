#ifndef NET_DNS_TASK_RESULT_I_H_INCLEDED
#define NET_DNS_TASK_RESULT_I_H_INCLEDED
#include "net_dns_entry_item.h"
#include "net_dns_entry_i.h"

NET_BEGIN_DECL

struct net_dns_entry_item {
    net_dns_entry_t m_entry;
    TAILQ_ENTRY(net_dns_entry_item) m_next_for_entry;
    net_address_t m_address;
};

net_dns_entry_item_t
net_dns_entry_item_create(
    net_dns_entry_t entry, net_address_t address, uint8_t is_own);

void net_dns_entry_item_free(net_dns_entry_item_t item);

void net_dns_entry_item_real_free(net_dns_entry_item_t entry_item);

net_dns_entry_item_t net_dns_entry_item_find(net_dns_entry_t entry, net_address_t address);

NET_END_DECL

#endif
