#ifndef NET_DNS_SVR_PROTOCOL_I_H_INCLEDED
#define NET_DNS_SVR_PROTOCOL_I_H_INCLEDED
#include "net_dns_svr_i.h"

NET_BEGIN_DECL

struct net_dns_svr_protocol {
    net_dns_svr_t m_svr;
};

net_protocol_t net_dns_svr_protocol_create(net_dns_svr_t dns_svr);

NET_END_DECL

#endif
