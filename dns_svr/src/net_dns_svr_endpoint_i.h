#ifndef NET_DNS_SVR_ENDPOINT_I_H_INCLEDED
#define NET_DNS_SVR_ENDPOINT_I_H_INCLEDED
#include "net_dns_svr_i.h"

NET_BEGIN_DECL

struct net_dns_svr_endpoint {
    net_dns_svr_itf_t m_itf;
};

int net_dns_svr_endpoint_init(net_endpoint_t base_endpoint);
void net_dns_svr_endpoint_fini(net_endpoint_t base_endpoint);
int net_dns_svr_endpoint_input(net_endpoint_t base_endpoint);
int net_dns_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from);

net_dns_svr_endpoint_t net_dns_svr_endpoint_get(net_endpoint_t base_endpoint);
void net_dns_svr_endpoint_set_itf(net_dns_svr_endpoint_t endpoint, net_dns_svr_itf_t dns_itf);

NET_END_DECL

#endif
