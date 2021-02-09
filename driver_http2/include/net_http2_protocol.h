#ifndef NET_HTTP2_PROTOCOL_H_INCLEDED
#define NET_HTTP2_PROTOCOL_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

net_http2_protocol_t
net_http2_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em);

void net_http2_protocol_free(net_http2_protocol_t protocol);

net_http2_protocol_t
net_http2_protocol_find(net_schedule_t schedule, const char * addition_name);

net_http2_protocol_t net_http2_protocol_cast(net_protocol_t protocol);

NET_END_DECL

#endif
