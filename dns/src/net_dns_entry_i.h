#ifndef NET_DNS_ENTRY_I_H_INCLEDED
#define NET_DNS_ENTRY_I_H_INCLEDED
#include "net_dns_entry.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_entry {
    const char * m_hostname;
    union {
        struct cpe_hash_entry m_hh;
        TAILQ_ENTRY(net_dns_entry) m_next;
    };
    net_address_t m_address;
    uint32_t m_expire_time_ms;
    char m_hostname_buf[32];
};

void net_dns_entry_real_free(net_dns_manage_t manage, net_dns_entry_t entry);
void net_dns_entry_free_all(net_dns_manage_t manage);

uint32_t net_dns_entry_hash(net_dns_entry_t o);
int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r);

NET_END_DECL

#endif
