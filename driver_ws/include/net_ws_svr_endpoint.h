#ifndef NET_WS_SVR_ENDPOINT_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_endpoint_t
net_ws_svr_endpoint_create(
    net_ws_svr_driver_t driver, net_protocol_t protocol);

net_endpoint_t net_ws_svr_endpoint_underline(net_endpoint_t ws_endpoint);

NET_END_DECL

#endif
