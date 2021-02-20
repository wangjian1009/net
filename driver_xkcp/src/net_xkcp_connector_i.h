#ifndef NET_XKCP_CONNECTOR_I_H_INCLEDED
#define NET_XKCP_CONNECTOR_I_H_INCLEDED
#include "net_xkcp_connector.h"
#include "net_xkcp_driver_i.h"

struct net_xkcp_connector {
    net_xkcp_driver_t m_driver;
    cpe_hash_entry m_hh_for_driver;

    net_address_t m_remote_address;
    net_dgram_t m_dgram;

    struct cpe_hash_table m_streams;
};

int net_xkcp_connector_eq(net_xkcp_connector_t l, net_xkcp_connector_t r, void * user_data);
uint32_t net_xkcp_connector_hash(net_xkcp_connector_t o, void * user_data);

#endif
