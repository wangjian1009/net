#ifndef NET_DNS_ENTRY_I_H_INCLEDED
#define NET_DNS_ENTRY_I_H_INCLEDED
#include "net_dns_entry.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_entry {
    net_dns_manage_t m_manage;
    const char * m_hostname;
    union {
        struct cpe_hash_entry m_hh;
        TAILQ_ENTRY(net_dns_entry) m_next;
    };
    net_dns_task_t m_task;
    net_dns_entry_t m_main;
    TAILQ_ENTRY(net_dns_entry) m_next_for_main;
    net_dns_entry_list_t m_cnames;
    net_dns_entry_item_list_t m_items;
    char m_hostname_buf[32]; /* 必须放在最后，可能需要延展 */
};

void net_dns_entry_real_free(net_dns_entry_t entry);
void net_dns_entry_free_all(net_dns_manage_t manage);

uint32_t net_dns_entry_hash(net_dns_entry_t o, void * user_data);
int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r, void * user_data);

NET_END_DECL

#endif
