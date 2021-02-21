#ifndef NET_XKCP_ENDPOINT_H_INCLEDED
#define NET_XKCP_ENDPOINT_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

enum net_xkcp_endpoint_runing_mode {
    net_xkcp_endpoint_runing_mode_init,
    net_xkcp_endpoint_runing_mode_cli,
    net_xkcp_endpoint_runing_mode_svr,
};

net_xkcp_endpoint_t
net_xkcp_endpoint_create(
    net_xkcp_driver_t driver, net_protocol_t protocol);

net_xkcp_endpoint_t net_xkcp_endpoint_cast(net_endpoint_t base_endpoint);
net_endpoint_t net_xkcp_endpoint_base_endpoint(net_xkcp_endpoint_t endpoint);

uint32_t net_xkcp_endpoint_conv(net_xkcp_endpoint_t endpoint);
net_xkcp_endpoint_runing_mode_t net_xkcp_endpoint_runing_mode(net_xkcp_endpoint_t endpoint);

net_xkcp_connector_t net_xkcp_endpoint_connector(net_xkcp_endpoint_t endpoint);

NET_END_DECL

#endif
