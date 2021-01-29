#ifndef NET_WS_CLI_DRIVER_H_INCLEDED
#define NET_WS_CLI_DRIVER_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_driver_t
net_ws_cli_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em);

void net_ws_cli_driver_free(net_ws_cli_driver_t cli_driver);

net_ws_cli_driver_t
net_ws_cli_driver_find(net_schedule_t schedule, const char * addition_name);

NET_END_DECL

#endif
