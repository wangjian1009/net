#ifndef NET_DNS_QUERY_EX_H_INCLEDED
#define NET_DNS_QUERY_EX_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_query_ex {
    net_dns_manage_t m_manage;
    net_dns_task_t m_task;
    net_dns_query_type_t m_query_type;
    net_address_t m_address;
    TAILQ_ENTRY(net_dns_query_ex) m_next;
};

void net_dns_query_ex_set_task(net_dns_query_ex_t query_ex, net_dns_task_t task);
    
int net_dns_query_ex_init(void * ctx, net_dns_query_t query, net_address_t hostname, net_dns_query_type_t query_type, const char * policy);
void net_dns_query_ex_fini(void * ctx, net_dns_query_t query);

NET_END_DECL

#endif
