#ifndef NET_KCP_SMUX_I_H_INCLEDED
#define NET_KCP_SMUX_I_H_INCLEDED
#include "net_kcp_driver_i.h"

struct net_kcp_smux {
    net_kcp_driver_t m_driver;
    TAILQ_ENTRY(net_kcp_smux) m_next;
    net_dgram_t m_dgram;
};

net_kcp_smux_t net_kcp_smux_create(net_kcp_driver_t driver, net_address_t address);
void net_kcp_smux_free(net_kcp_smux_t mux);

#endif

