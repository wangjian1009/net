#ifndef NET_DRIVER_H_INCLEDED
#define NET_DRIVER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef int (*net_driver_init_fun_t)(net_driver_t driver);
typedef void (*net_driver_fini_fun_t)(net_driver_t driver);

typedef int (*net_timer_init_fun_t)(net_timer_t timer);
typedef void (*net_timer_fini_fun_t)(net_timer_t timer);
typedef void (*net_timer_schedule_fun_t)(net_timer_t timer, uint64_t delay_milliseconds);
typedef void (*net_timer_cancel_fun_t)(net_timer_t timer);
typedef uint8_t (*net_timer_is_active_fun_t)(net_timer_t timer);

typedef int (*net_acceptor_init_fun_t)(net_acceptor_t acceptor);
typedef void (*net_acceptor_fini_fun_t)(net_acceptor_t acceptor);

typedef int (*net_endpoint_init_fun_t)(net_endpoint_t endpoint);
typedef void (*net_endpoint_fini_fun_t)(net_endpoint_t endpoint);
typedef int (*net_endpoint_update_fun_t)(net_endpoint_t endpoint);
typedef int (*net_endpoint_connect_fun_t)(net_endpoint_t endpoint);
typedef void (*net_endpoint_close_fun_t)(net_endpoint_t endpoint);
typedef int (*net_endpoint_update_option_fun_t)(net_endpoint_t endpoint, net_endpoint_option_t opt, uint8_t is_enable);

typedef int (*net_dgram_init_fun_t)(net_dgram_t dgram);
typedef void (*net_dgram_fini_fun_t)(net_dgram_t dgram);
typedef int (*net_dgram_send_fun_t)(net_dgram_t dgram, net_address_t target, void const * data, size_t data_len);

typedef int (*net_watcher_init_fun_t)(net_watcher_t watcher, int fd);
typedef void (*net_watcher_fini_fun_t)(net_watcher_t watcher, int fd);
typedef void (*net_watcher_update_fun_t)(net_watcher_t watcher, int fd, uint8_t expect_read, uint8_t expect_write);

net_driver_t
net_driver_create(
    net_schedule_t schedule,
    const char * driver_name,
    /*driver*/
    uint16_t driver_capacity,
    net_driver_init_fun_t driver_init,
    net_driver_fini_fun_t driver_fini,
    /*timer*/
    uint16_t timer_capacity,
    net_timer_init_fun_t timer_init,
    net_timer_fini_fun_t timer_fini,
    net_timer_schedule_fun_t timer_schedule,
    net_timer_cancel_fun_t timer_cancel,
    net_timer_is_active_fun_t timer_is_active,
    /*acceptor*/
    uint16_t acceptor_capacity,
    net_acceptor_init_fun_t acceptor_init,
    net_acceptor_fini_fun_t acceptor_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_endpoint_init_fun_t endpoint_init,
    net_endpoint_fini_fun_t endpoint_fini,
    net_endpoint_connect_fun_t endpoint_connect,
    net_endpoint_close_fun_t endpoint_close,
    net_endpoint_update_fun_t endpoint_update,
    net_endpoint_update_option_fun_t endpoint_update_option,
    /*dgram*/
    uint16_t dgram_capacity,
    net_dgram_init_fun_t dgram_init,
    net_dgram_fini_fun_t dgram_fini,
    net_dgram_send_fun_t dgram_send,
    /*watcher*/
    uint16_t watcher_capacity,
    net_watcher_init_fun_t watcher_init,
    net_watcher_fini_fun_t watcher_fini,
    net_watcher_update_fun_t watcher_update);

void net_driver_free(net_driver_t driver);

net_driver_t
net_driver_find(net_schedule_t schedule, const char * driver_name);

net_schedule_t net_driver_schedule(net_driver_t driver);
const char * net_driver_name(net_driver_t driver);

void * net_driver_data(net_driver_t driver);
net_driver_t net_driver_from_data(void * data);

uint8_t net_driver_debug(net_driver_t driver);
void net_driver_set_debug(net_driver_t driver, uint8_t debug);

NET_END_DECL

#endif
