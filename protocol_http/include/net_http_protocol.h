#ifndef NET_HTTP_PROTOCOL_H_INCLEDED
#define NET_HTTP_PROTOCOL_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_protocol_t net_http_protocol_create(net_schedule_t schedule);
void net_http_protocol_free(net_http_protocol_t http_protocol);

NET_END_DECL

#endif
