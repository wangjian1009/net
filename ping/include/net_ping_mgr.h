#ifndef NET_PING_MGR_H_INCLEDED
#define NET_PING_MGR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ping_types.h"

NET_BEGIN_DECL

net_ping_mgr_t net_ping_mgr_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver);
void net_ping_mgr_free(net_ping_mgr_t mgr);

NET_END_DECL

#endif
