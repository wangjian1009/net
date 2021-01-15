#ifndef NET_SSL_SVR_PROTOCOL_H_INCLEDED
#define NET_SSL_SVR_PROTOCOL_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_ssl_svr_protocol_t
net_ssl_svr_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em);

void net_ssl_svr_protocol_free(net_ssl_svr_protocol_t svr_protocol);

net_ssl_svr_protocol_t
net_ssl_svr_protocol_find(net_schedule_t schedule, const char * addition_name);

NET_END_DECL

#endif
