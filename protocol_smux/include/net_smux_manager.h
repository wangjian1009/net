#ifndef NET_SMUX_MANAGER_H_INCLEDED
#define NET_SMUX_MANAGER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_smux_types.h"

NET_BEGIN_DECL

net_smux_manager_t
net_smux_manager_create(
    net_schedule_t schedule, mem_allocrator_t alloc, error_monitor_t em);

void net_smux_manager_free(net_smux_manager_t manager);

NET_END_DECL

#endif
