#ifndef NET_DNS_SERVER_I_H_INCLEDED
#define NET_DNS_SERVER_I_H_INCLEDED
#include "net_dns_server.h"
#include "net_dns_resolver_i.h"

NET_BEGIN_DECL

struct net_dns_server {
    net_dns_resolver_t m_resolver;
    TAILQ_ENTRY(net_dns_server) m_next;
    net_address_t m_address;
};

NET_END_DECL

#endif
