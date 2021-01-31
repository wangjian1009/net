#ifndef NET_WS_SVR_ENDPOINT_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

enum net_ws_svr_endpoint_state {
    net_ws_svr_endpoint_state_handshake,
    net_ws_svr_endpoint_state_streaming,
};

net_ws_svr_endpoint_t net_ws_svr_endpoint_cast(net_endpoint_t endpoint);
net_endpoint_t net_ws_svr_endpoint_stream(net_endpoint_t endpoint);

const char * net_ws_svr_endpoint_path(net_ws_svr_endpoint_t endpoint);
net_address_t net_ws_svr_endpoint_host(net_ws_svr_endpoint_t endpoint);
uint8_t net_ws_svr_endpoint_version(net_ws_svr_endpoint_t endpoint);

net_ws_svr_endpoint_state_t net_ws_svr_endpoint_state(net_ws_svr_endpoint_t endpoint);

const char * net_ws_svr_endpoint_state_str(net_ws_svr_endpoint_state_t state);

NET_END_DECL

#endif
