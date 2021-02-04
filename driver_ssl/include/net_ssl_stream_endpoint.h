#ifndef NET_SSL_STREAM_ENDPOINT_H_INCLEDED
#define NET_SSL_STREAM_ENDPOINT_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_ssl_stream_endpoint_t
net_ssl_stream_endpoint_create(
    net_ssl_stream_driver_t driver, net_protocol_t protocol);

net_ssl_stream_endpoint_t net_ssl_stream_endpoint_cast(net_endpoint_t base_endpoint);
net_endpoint_t net_ssl_stream_endpoint_underline(net_endpoint_t base_endpoint);

net_endpoint_t net_ssl_stream_endpoint_base_endpoint(net_ssl_stream_endpoint_t endpoint);

net_ssl_endpoint_runing_mode_t
net_ssl_stream_endpoint_runing_mode(net_ssl_stream_endpoint_t endpoint);

int net_ssl_stream_endpoint_set_runing_mode(
    net_ssl_stream_endpoint_t endpoint, net_ssl_endpoint_runing_mode_t runing_mode);

NET_END_DECL

#endif
