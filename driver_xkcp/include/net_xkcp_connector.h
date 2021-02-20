#ifndef NET_XKCP_CONNECTOR_H_INCLEDED
#define NET_XKCP_CONNECTOR_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

net_xkcp_connector_t
net_xkcp_connector_create(
    net_xkcp_driver_t driver, net_address_t remote_address, net_xkcp_config_t config);

void net_xkcp_connector_free(net_xkcp_connector_t connector);

net_xkcp_connector_t
net_xkcp_connector_find(net_xkcp_driver_t driver, net_address_t remote_address);

net_dgram_t net_xkcp_connector_dgram(net_xkcp_connector_t connector);

NET_END_DECL

#endif
