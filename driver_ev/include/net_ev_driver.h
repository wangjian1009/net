#ifndef NET_EV_DRIVER_H_INCLEDED
#define NET_EV_DRIVER_H_INCLEDED
#include "ev.h"
#include "cpe/utils/utils_types.h"
#include "net_ev_types.h"

NET_BEGIN_DECL

net_ev_driver_t net_ev_driver_create(
    net_schedule_t schedule, struct ev_loop * ev_loop);

void net_ev_driver_free(net_ev_driver_t driver);

net_ev_driver_t net_ev_driver_cast(net_driver_t base_driver);
struct ev_loop * net_ev_driver_loop(net_ev_driver_t driver);

typedef int (*net_ev_driver_sock_create_process_fun_t)(
    net_ev_driver_t driver, void * ctx, int fd, net_address_t remote_addr);

void net_ev_driver_set_sock_create_processor(
    net_ev_driver_t driver,
    net_ev_driver_sock_create_process_fun_t process_fun, void * process_ctx);

NET_END_DECL

#endif
