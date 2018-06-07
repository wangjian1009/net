#ifndef NET_DNS_QUERY_EX_H_INCLEDED
#define NET_DNS_QUERY_EX_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_query_ex {
    net_dns_manage_t m_manage;
    TAILQ_ENTRY(net_dns_query_ex) m_next;
};

int net_dns_query_ex_start(void * ctx, net_dns_query_t query);
void net_dns_query_ex_cancel(void * ctx, net_dns_query_t query);

NET_END_DECL

#endif
