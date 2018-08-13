#ifndef NET_DNS_SVR_ENDPOINT_I_H_INCLEDED
#define NET_DNS_SVR_ENDPOINT_I_H_INCLEDED
#include "net_dns_svr_i.h"

NET_BEGIN_DECL

struct net_dns_svr_endpoint {
    net_endpoint_t m_endpoint;
};

int net_dns_svr_endpoint_init(net_endpoint_t endpoint);
void net_dns_svr_endpoint_fini(net_endpoint_t endpoint);
int net_dns_svr_endpoint_input(net_endpoint_t endpoint);

NET_END_DECL

#endif
