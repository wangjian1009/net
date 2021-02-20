#ifndef NET_KCP_CONNECTOR_H_INCLEDED
#define NET_KCP_CONNECTOR_H_INCLEDED
#include "net_kcp_types.h"

NET_BEGIN_DECL

net_kcp_connector_t
net_kcp_connector_create(
    net_kcp_driver_t driver, net_address_t remote_address, net_kcp_config_t config);

void net_kcp_connector_free(net_kcp_connector_t connector);

net_kcp_connector_t
net_kcp_connector_find(net_kcp_driver_t driver, net_address_t remote_address);

NET_END_DECL

#endif
