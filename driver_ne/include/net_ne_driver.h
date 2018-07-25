#ifndef NET_NE_DRIVER_H_INCLEDED
#define NET_NE_DRIVER_H_INCLEDED
#import <NetworkExtension/NETunnelProvider.h>
#include "cpe/utils/utils_types.h"
#include "net_ne_types.h"

NET_BEGIN_DECL

net_ne_driver_t net_ne_driver_create(net_schedule_t schedule, NETunnelProvider * tunnel_provider);

void net_ne_driver_free(net_ne_driver_t driver);

uint8_t net_ne_driver_debug(net_ne_driver_t driver);
void net_ne_driver_set_debug(net_ne_driver_t driver, uint8_t debug);

void net_ne_driver_set_data_monitor(
    net_ne_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx);

NET_END_DECL

#endif
