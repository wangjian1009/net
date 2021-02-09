#ifndef NET_HTTP2_STREAM_GROUP_H_INCLEDED
#define NET_HTTP2_STREAM_GROUP_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

net_http2_stream_group_t
net_http2_stream_group_create(net_http2_stream_driver_t driver, net_address_t address);

void net_http2_stream_group_free(net_http2_stream_group_t remote);

net_http2_stream_group_t
net_http2_stream_group_check_create(net_http2_stream_driver_t driver, net_address_t address);

net_http2_stream_group_t
net_http2_stream_group_find(net_http2_stream_driver_t driver, net_address_t address);

NET_END_DECL

#endif
