#ifndef NET_XKCP_DRIVER_H_INCLEDED
#define NET_XKCP_DRIVER_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

net_xkcp_driver_t
net_xkcp_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em);

void net_xkcp_driver_free(net_xkcp_driver_t stream_driver);

net_xkcp_driver_t net_xkcp_driver_cast(net_driver_t base_driver);

net_xkcp_driver_t
net_xkcp_driver_find(net_schedule_t schedule, const char * addition_name);

NET_END_DECL

#endif
