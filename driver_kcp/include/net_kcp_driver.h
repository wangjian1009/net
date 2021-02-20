#ifndef NET_WS_KCP_DRIVER_H_INCLEDED
#define NET_WS_KCP_DRIVER_H_INCLEDED
#include "net_kcp_types.h"

NET_BEGIN_DECL

net_kcp_driver_t
net_kcp_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em);

void net_kcp_driver_free(net_kcp_driver_t stream_driver);

net_kcp_driver_t net_kcp_driver_cast(net_driver_t base_driver);

net_kcp_driver_t
net_kcp_driver_find(net_schedule_t schedule, const char * addition_name);

NET_END_DECL

#endif
