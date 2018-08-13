#ifndef NET_DNS_SVR_QUERY_I_H_INCLEDED
#define NET_DNS_SVR_QUERY_I_H_INCLEDED
#include "net_dns_svr_itf_i.h"

NET_BEGIN_DECL

struct net_dns_svr_query {
    net_dns_svr_itf_t m_itf;
    TAILQ_ENTRY(net_dns_svr_query) m_next;
};

net_dns_svr_query_t net_dns_svr_query_create(net_dns_svr_itf_t dns_itf);
void net_dns_svr_query_free(net_dns_svr_query_t query);

void net_dns_svr_query_real_free(net_dns_svr_query_t query);

NET_END_DECL

#endif


