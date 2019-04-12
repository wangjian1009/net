#ifndef NET_DNS_SVR_QUERY_ENTRY_I_H_INCLEDED
#define NET_DNS_SVR_QUERY_ENTRY_I_H_INCLEDED
#include "net_dns_svr_query_i.h"

NET_BEGIN_DECL

typedef enum net_dns_svr_query_entry_type {
    net_dns_svr_query_entry_type_ipv4,
    net_dns_svr_query_entry_type_ipv6,
    net_dns_svr_query_entry_type_ptr,
} net_dns_svr_query_entry_type_t;

struct net_dns_svr_query_entry {
    net_dns_svr_query_t m_query;
    TAILQ_ENTRY(net_dns_svr_query_entry) m_next;
    net_dns_svr_query_entry_type_t m_type;
    char m_domain_name[128];
    net_dns_query_t m_local_query;
    uint8_t m_result_count;
    net_address_t m_results[5];
    uint16_t m_cache_name_offset;
};

net_dns_svr_query_entry_t net_dns_svr_query_entry_create(
    net_dns_svr_query_t query, const char * domain_name, net_dns_svr_query_entry_type_t type);
void net_dns_svr_query_entry_free(net_dns_svr_query_entry_t query_entry);

uint8_t net_dns_svr_query_entry_have_address(net_dns_svr_query_entry_t query_entry, net_address_t address);

void net_dns_svr_query_entry_real_free(net_dns_svr_query_entry_t query_entry);

int net_dns_svr_query_entry_start(net_dns_svr_query_entry_t query_entry);

uint16_t net_dns_svr_query_entry_type_to_atype(net_dns_svr_query_entry_type_t entry_type);

NET_END_DECL

#endif


