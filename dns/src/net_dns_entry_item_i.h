#ifndef NET_DNS_TASK_RESULT_I_H_INCLEDED
#define NET_DNS_TASK_RESULT_I_H_INCLEDED
#include "net_dns_entry_item.h"
#include "net_dns_entry_i.h"

NET_BEGIN_DECL

struct net_dns_entry_item {
    net_dns_entry_t m_entry;
    TAILQ_ENTRY(net_dns_entry_item) m_next_for_entry;
    net_dns_source_t m_source;
    TAILQ_ENTRY(net_dns_entry_item) m_next_for_source;
    net_address_t m_address;
    uint32_t m_expire_time_ms;
};

void net_dns_entry_item_real_free(net_dns_entry_item_t entry_item);

NET_END_DECL

#endif
