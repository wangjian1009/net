#ifndef NET_DNS_QUERY_I_H_INCLEDED
#define NET_DNS_QUERY_I_H_INCLEDED
#include "net_dns_query.h"
#include "net_dns_resolver_i.h"

NET_BEGIN_DECL

struct net_dns_query {
    net_dns_resolver_t m_resolver;
    union {
        TAILQ_ENTRY(net_dns_query) m_next;
        struct cpe_hash_entry m_hh;
    };
    uint32_t m_query_id;
    net_dns_query_callback_fun_t m_callback;
    net_dns_query_ctx_free_fun_t m_ctx_free;
    void * m_ctx;
};

void net_dns_query_real_free(net_dns_query_t query);
void net_dns_query_free_all(net_dns_resolver_t resolver);

uint32_t net_dns_query_hash(net_dns_query_t o);
int net_dns_query_eq(net_dns_query_t l, net_dns_query_t r);

NET_END_DECL

#endif
