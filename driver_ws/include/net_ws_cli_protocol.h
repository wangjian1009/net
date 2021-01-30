#ifndef NET_WS_CLI_PROTOCOL_H_INCLEDED
#define NET_WS_CLI_PROTOCOL_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_protocol_t
net_ws_cli_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em);

void net_ws_cli_protocol_free(net_ws_cli_protocol_t protocol);

net_ws_cli_protocol_t
net_ws_cli_protocol_find(net_schedule_t schedule, const char * addition_name);

net_ws_cli_protocol_t net_ws_cli_protocol_cast(net_protocol_t protocol);

NET_END_DECL

#endif
