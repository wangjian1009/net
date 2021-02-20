#ifndef NET_XKCP_ACCEPTOR_I_H_INCLEDED
#define NET_XKCP_ACCEPTOR_I_H_INCLEDED
#include "net_xkcp_acceptor.h"

struct net_xkcp_acceptor {
    net_xkcp_config_t m_config;
    net_dgram_t m_dgram;
};

int net_xkcp_acceptor_init(net_acceptor_t acceptor);
void net_xkcp_acceptor_fini(net_acceptor_t acceptor);

#endif
