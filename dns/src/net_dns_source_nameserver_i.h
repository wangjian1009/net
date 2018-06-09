#ifndef NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#define NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#include "net_dns_source.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_source_nameserver {
    net_address_t m_address;
    net_endpoint_t m_endpoint;
};

NET_END_DECL

#endif
