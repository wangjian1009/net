#ifndef NET_HTTP2_ENDPOINT_H_INCLEDED
#define NET_HTTP2_ENDPOINT_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

net_http2_endpoint_t
net_http2_endpoint_create(
    net_http2_driver_t driver, net_protocol_t protocol);

net_http2_endpoint_t net_http2_endpoint_cast(net_endpoint_t base_endpoint);

net_http2_control_endpoint_t net_http2_endpoint_control(net_http2_endpoint_t endpoint);
net_endpoint_t net_http2_endpoint_base_endpoint(net_http2_endpoint_t endpoint);

NET_END_DECL

#endif
