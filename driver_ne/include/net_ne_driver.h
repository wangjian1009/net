#ifndef NET_NE_DRIVER_H_INCLEDED
#define NET_NE_DRIVER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ne_types.h"

NET_BEGIN_DECL

net_ne_driver_t net_ne_driver_create(net_schedule_t schedule, /*NETunnelProvider*/void * tunnel_provider);

void net_ne_driver_free(net_ne_driver_t driver);

uint8_t net_ne_driver_debug(net_ne_driver_t driver);
void net_ne_driver_set_debug(net_ne_driver_t driver, uint8_t debug);

typedef int (*net_ne_driver_sock_create_process_fun_t)(
    net_ne_driver_t driver, void * ctx, int fd, net_address_t remote_addr);

void net_ne_driver_set_sock_create_processor(
    net_ne_driver_t driver,
    net_ne_driver_sock_create_process_fun_t process_fun, void * process_ctx);

void net_ne_driver_set_data_monitor(
    net_ne_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx);

NET_END_DECL

#endif
