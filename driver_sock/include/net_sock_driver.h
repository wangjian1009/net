#ifndef NET_SOCK_DRIVER_H_INCLEDED
#define NET_SOCK_DRIVER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_driver.h"
#include "net_sock_types.h"

NET_BEGIN_DECL

typedef int (*net_sock_driver_init_fun_t)(net_sock_driver_t sock_driver);
typedef void (*net_sock_driver_fini_fun_t)(net_sock_driver_t sock_driver);

net_sock_driver_t net_sock_driver_create(
    net_schedule_t schedule,
    const char * driver_name,
    /*driver*/
    uint16_t driver_capacity,
    net_sock_driver_init_fun_t driver_init,
    net_sock_driver_fini_fun_t driver_fini,
    /*timer*/
    uint16_t timer_capacity,
    net_timer_init_fun_t timer_init,
    net_timer_fini_fun_t timer_fini,
    net_timer_schedule_fun_t timer_schedule,
    net_timer_cancel_fun_t timer_cancel,
    net_timer_is_active_fun_t timer_is_active,
    /*watcher*/
    uint16_t watcher_capacity,
    net_watcher_init_fun_t watcher_init,
    net_watcher_fini_fun_t watcher_fini,
    net_watcher_update_fun_t watcher_update);

void net_sock_driver_free(net_sock_driver_t sock_driver);

net_schedule_t net_sock_driver_schedule(net_sock_driver_t sock_driver);
net_driver_t net_sock_driver_base_driver(net_sock_driver_t sock_driver);

void * net_sock_driver_data(net_sock_driver_t sock_driver);
void * net_sock_driver_data_from_base_driver(net_driver_t base_driver);

net_sock_driver_t net_sock_driver_from_data(void * data);

typedef int (*net_sock_driver_sock_create_process_fun_t)(
    net_sock_driver_t driver, void * ctx, int fd, net_address_t remote_addr);

void net_sock_driver_set_sock_create_processor(
    net_sock_driver_t driver,
    net_sock_driver_sock_create_process_fun_t process_fun, void * process_ctx);

NET_END_DECL

#endif
