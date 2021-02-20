#ifndef NET_XKCP_ACCEPTOR_I_H_INCLEDED
#define NET_XKCP_ACCEPTOR_I_H_INCLEDED
#include "net_xkcp_acceptor.h"
#include "net_xkcp_config.h"
#include "net_xkcp_driver_i.h"

struct net_xkcp_acceptor {
    struct net_xkcp_config m_config;
    net_dgram_t m_dgram;
    struct cpe_hash_table m_clients;
};

int net_xkcp_acceptor_init(net_acceptor_t acceptor);
void net_xkcp_acceptor_fini(net_acceptor_t acceptor);

#endif
