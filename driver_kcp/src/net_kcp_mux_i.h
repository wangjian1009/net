#ifndef NET_KCP_MUX_I_H_INCLEDED
#define NET_KCP_MUX_I_H_INCLEDED
#include "net_kcp_driver_i.h"

struct net_kcp_mux {
    net_kcp_driver_t m_driver;
    TAILQ_ENTRY(net_kcp_mux) m_next;
    net_dgram_t m_dgram;
};

net_kcp_mux_t net_kcp_mux_create(net_kcp_driver_t driver, net_address_t address);
void net_kcp_mux_free(net_kcp_mux_t mux);

#endif

