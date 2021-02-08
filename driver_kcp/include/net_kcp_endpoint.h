#ifndef NET_KCP_ENDPOINT_H_INCLEDED
#define NET_KCP_ENDPOINT_H_INCLEDED
#include "net_kcp_types.h"

NET_BEGIN_DECL

net_kcp_endpoint_t
net_kcp_endpoint_create(
    net_kcp_driver_t driver, net_protocol_t protocol);

net_kcp_endpoint_t net_kcp_endpoint_cast(net_endpoint_t base_endpoint);
net_endpoint_t net_kcp_endpoint_base_endpoint(net_kcp_endpoint_t endpoint);


NET_END_DECL

#endif
