#ifndef NET_WS_CLI_STREAM_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_STREAM_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_stream_endpoint_t
net_ws_stream_endpoint_create(
    net_ws_stream_driver_t driver, net_protocol_t protocol);

net_ws_stream_endpoint_t net_ws_stream_endpoint_cast(net_endpoint_t base_endpoint);
net_endpoint_t net_ws_stream_endpoint_underline(net_endpoint_t base_endpoint);

net_endpoint_t net_ws_stream_endpoint_base_endpoint(net_ws_stream_endpoint_t endpoint);

const char * net_ws_stream_endpoint_path(net_ws_stream_endpoint_t endpoint);
int net_ws_stream_endpoint_set_path(net_ws_stream_endpoint_t endpoint, const char * path);

net_address_t net_ws_stream_endpoint_host(net_ws_stream_endpoint_t endpoint);
int net_ws_stream_endpoint_set_host(net_ws_stream_endpoint_t endpoint, net_address_t host);

NET_END_DECL

#endif
