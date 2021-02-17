#ifndef NET_SMUX_PROTOCOL_H_INCLEDED
#define NET_SMUX_PROTOCOL_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_smux_types.h"

NET_BEGIN_DECL

net_smux_protocol_t
net_smux_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em);

void net_smux_protocol_free(net_smux_protocol_t protocol);

net_smux_protocol_t
net_smux_protocol_find(net_schedule_t schedule, const char * addition_name);

net_smux_protocol_t net_smux_protocol_cast(net_protocol_t protocol);

NET_END_DECL

#endif
