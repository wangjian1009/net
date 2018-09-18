#ifndef NET_TRANS_MANAGE_H_INCLEDED
#define NET_TRANS_MANAGE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_trans_system.h"

NET_BEGIN_DECL

net_trans_manage_t net_trans_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver);

void net_trans_manage_free(net_trans_manage_t trans_manage);

uint8_t net_trans_manage_debug(net_trans_manage_t manage);
void net_trans_manage_set_debug(net_trans_manage_t manage, uint8_t debug);

NET_END_DECL

#endif
