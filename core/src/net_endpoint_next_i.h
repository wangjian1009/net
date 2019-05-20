#ifndef NET_ENDPOINT_NEXT_I_H_INCLEDED
#define NET_ENDPOINT_NEXT_I_H_INCLEDED
#include "net_endpoint_i.h"

NET_BEGIN_DECL

struct net_endpoint_next {
    net_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_endpoint_next) m_next;
    net_address_t m_address;
};

net_endpoint_next_t net_endpoint_next_create(net_endpoint_t endpoint, net_address_t address);
void net_endpoint_next_free(net_endpoint_next_t endpoint_next);

void net_endpoint_next_real_free(net_endpoint_next_t endpoint_next);

NET_END_DECL

#endif
