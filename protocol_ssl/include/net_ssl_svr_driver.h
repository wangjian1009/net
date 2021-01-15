#ifndef NET_SSL_SVR_DRIVER_H_INCLEDED
#define NET_SSL_SVR_DRIVER_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_ssl_svr_driver_t
net_ssl_svr_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em);

void net_ssl_svr_driver_free(net_ssl_svr_driver_t svr_driver);

net_ssl_svr_driver_t
net_ssl_svr_driver_find(net_schedule_t schedule, const char * addition_name);

NET_END_DECL

#endif
