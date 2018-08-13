#ifndef NET_DNS_SVR_QUERY_I_H_INCLEDED
#define NET_DNS_SVR_QUERY_I_H_INCLEDED
#include "net_dns_svr_itf_i.h"

NET_BEGIN_DECL

struct net_dns_svr_query {
    net_dns_svr_itf_t m_itf;
    TAILQ_ENTRY(net_dns_svr_query) m_next;
    uint16_t m_trans_id;
    uint16_t m_runing_entry_count;
    net_dns_svr_query_entry_list_t m_entries;
    net_address_t m_source_addr;
    net_endpoint_t m_endpoint;
};

net_dns_svr_query_t net_dns_svr_query_create(net_dns_svr_itf_t dns_itf, uint16_t trans_id);
void net_dns_svr_query_free(net_dns_svr_query_t query);

void net_dns_svr_query_real_free(net_dns_svr_query_t query);

int net_dns_svr_query_start(net_dns_svr_query_t query);

net_dns_svr_query_t net_dns_svr_query_parse_request(net_dns_svr_itf_t itf, void const * data, uint32_t data_size);
int net_dns_svr_query_build_response(net_dns_svr_query_t query, mem_buffer_t buffer);

const char * net_dns_svr_req_dump(mem_buffer_t buffer, char const * buf);

NET_END_DECL

#endif


