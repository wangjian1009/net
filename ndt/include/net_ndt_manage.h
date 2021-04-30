#ifndef NET_NDT_MANAGE_H_INCLEDED
#define NET_NDT_MANAGE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ndt_system.h"

NET_BEGIN_DECL

net_ndt_manage_t net_ndt_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule);

void net_ndt_manage_free(net_ndt_manage_t ndt_manage);

uint8_t net_ndt_manage_debug(net_ndt_manage_t manage);
void net_ndt_manage_set_debug(net_ndt_manage_t manage, uint8_t debug);

NET_END_DECL

#endif
