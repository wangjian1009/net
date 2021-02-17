#ifndef NET_KCP_ACCEPTOR_I_H_INCLEDED
#define NET_KCP_ACCEPTOR_I_H_INCLEDED
#include "net_kcp_acceptor.h"
#include "net_kcp_driver_i.h"

struct net_kcp_acceptor {
    net_dgram_t m_dgram;
};

int net_kcp_acceptor_init(net_acceptor_t acceptor);
void net_kcp_acceptor_fini(net_acceptor_t acceptor);

#endif
