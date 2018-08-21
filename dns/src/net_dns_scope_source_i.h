#ifndef NET_DNS_SCOPE_SOURCE_H_INCLEDED
#define NET_DNS_SCOPE_SOURCE_H_INCLEDED
#include "net_dns_scope_i.h"
#include "net_dns_source_i.h"

NET_BEGIN_DECL

struct net_dns_scope_source {
    net_dns_scope_t m_scope;
    TAILQ_ENTRY(net_dns_scope_source) m_next_for_scope;
    net_dns_source_t m_source;
    TAILQ_ENTRY(net_dns_scope_source) m_next_for_source;
};

net_dns_scope_source_t net_dns_scope_source_create(net_dns_scope_t scope, net_dns_source_t source);
void net_dns_scope_source_free(net_dns_scope_source_t scope_source);

void net_dns_scope_source_real_free(net_dns_scope_source_t scope_source);

NET_END_DECL

#endif
