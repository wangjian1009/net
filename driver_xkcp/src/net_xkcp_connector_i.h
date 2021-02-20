#ifndef NET_XKCP_CONNECTOR_I_H_INCLEDED
#define NET_XKCP_CONNECTOR_I_H_INCLEDED
#include "net_xkcp_connector.h"
#include "net_xkcp_driver_i.h"

struct net_xkcp_connector {
    net_xkcp_driver_t m_driver;
    cpe_hash_entry m_hh_for_driver;
    
};

#endif
