#ifndef NET_KCP_CONNECTOR_I_H_INCLEDED
#define NET_KCP_CONNECTOR_I_H_INCLEDED
#include "net_kcp_connector.h"
#include "net_kcp_driver_i.h"
#include "net_smux_dgram.h"

struct net_kcp_connector {
    net_kcp_driver_t m_driver;
    net_smux_dgram_t m_smux_dgram;
};

#endif
