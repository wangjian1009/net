#ifndef NET_DNS_SVR_QUERY_ENTRY_I_H_INCLEDED
#define NET_DNS_SVR_QUERY_ENTRY_I_H_INCLEDED
#include "net_dns_svr_query_i.h"

NET_BEGIN_DECL

struct net_dns_svr_query_entry {
    net_dns_svr_query_t m_query;
    TAILQ_ENTRY(net_dns_svr_query_entry) m_next;
    char m_domain_name[128];
};

net_dns_svr_query_entry_t net_dns_svr_query_entry_create(net_dns_svr_query_t query, const char * domain_name);
void net_dns_svr_query_entry_free(net_dns_svr_query_entry_t query_entry);

void net_dns_svr_query_entry_real_free(net_dns_svr_query_entry_t query_entry);

NET_END_DECL

#endif


