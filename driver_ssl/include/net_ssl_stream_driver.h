#ifndef NET_SSL_STREAM_DRIVER_H_INCLEDED
#define NET_SSL_STREAM_DRIVER_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_ssl_stream_driver_t
net_ssl_stream_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em);

void net_ssl_stream_driver_free(net_ssl_stream_driver_t stream_driver);

net_ssl_stream_driver_t
net_ssl_stream_driver_find(net_schedule_t schedule, const char * addition_name);

net_ssl_protocol_t net_ssl_stream_driver_underline_protocol(net_ssl_stream_driver_t driver);

int net_ssl_stream_driver_svr_confirm_pkey(net_ssl_stream_driver_t driver);
int net_ssl_stream_driver_svr_confirm_cert(net_ssl_stream_driver_t driver);
int net_ssl_stream_driver_svr_prepaired(net_ssl_stream_driver_t driver);

NET_END_DECL

#endif
