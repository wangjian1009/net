#ifndef NET_HTTP_PROTOCOL_H_INCLEDED
#define NET_HTTP_PROTOCOL_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_protocol_t net_http_protocol_create(net_schedule_t schedule, const char * name_postfix);
void net_http_protocol_free(net_http_protocol_t http_protocol);

net_http_protocol_t net_http_protocol_find(net_schedule_t schedule, const char * name_postfix);

NET_END_DECL

#endif
