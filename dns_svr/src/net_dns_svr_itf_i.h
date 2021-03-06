#ifndef NET_DNS_SVR_ITF_I_H_INCLEDED
#define NET_DNS_SVR_ITF_I_H_INCLEDED
#include "net_dns_svr_itf.h"
#include "net_dns_svr_i.h"

NET_BEGIN_DECL

struct net_dns_svr_itf {
    net_dns_svr_t m_svr;
    TAILQ_ENTRY(net_dns_svr_itf) m_next;
    net_dns_svr_itf_type_t m_type;
    net_dns_svr_query_list_t m_querys;
    char * m_query_policy;
    union {
        net_dgram_t m_dgram;
        net_acceptor_t m_acceptor;
    };
};

int net_dns_svr_itf_send_response(net_dns_svr_itf_t itf, net_dns_svr_query_t query);

NET_END_DECL

#endif


