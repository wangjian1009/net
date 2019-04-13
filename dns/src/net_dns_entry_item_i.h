#ifndef NET_DNS_TASK_RESULT_I_H_INCLEDED
#define NET_DNS_TASK_RESULT_I_H_INCLEDED
#include "net_dns_entry_item.h"
#include "net_dns_entry_i.h"

NET_BEGIN_DECL

struct net_dns_entry_item {
    net_dns_entry_t m_entry;
    TAILQ_ENTRY(net_dns_entry_item) m_next_for_entry;
    net_address_t m_address;
    cpe_hash_entry m_hh_for_ip;
};

net_dns_entry_item_t
net_dns_entry_item_create(
    net_dns_entry_t entry, net_address_t address, uint8_t is_own);

void net_dns_entry_item_free(net_dns_entry_item_t item);

void net_dns_entry_item_real_free(net_dns_entry_item_t entry_item);

net_dns_entry_item_t net_dns_entry_item_find(net_dns_entry_t entry, net_address_t address);

uint32_t net_dns_entry_item_hash_by_ip(net_dns_entry_item_t o, void * user_data);
int net_dns_entry_item_eq_by_ip(net_dns_entry_item_t l, net_dns_entry_item_t r, void * user_data);

NET_END_DECL

#endif
