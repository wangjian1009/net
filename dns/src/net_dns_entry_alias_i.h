#ifndef NET_DNS_ENTRY_ALIAS_I_H_INCLEDED
#define NET_DNS_ENTRY_ALIAS_I_H_INCLEDED
#include "net_dns_entry_i.h"

NET_BEGIN_DECL

struct net_dns_entry_alias {
    net_dns_entry_t m_origin;
    TAILQ_ENTRY(net_dns_entry_alias) m_next_for_origin;
    net_dns_entry_t m_as;
    TAILQ_ENTRY(net_dns_entry_alias) m_next_for_as;
};

net_dns_entry_alias_t
net_dns_entry_alias_create(net_dns_entry_t origin, net_dns_entry_t as);

void net_dns_entry_alias_free(net_dns_entry_alias_t alias);
void net_dns_entry_alias_real_free(net_dns_entry_alias_t alias);

NET_END_DECL

#endif
