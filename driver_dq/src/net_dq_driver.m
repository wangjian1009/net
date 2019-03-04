#include "net_schedule.h"
#include "net_driver.h"
#include "net_dq_driver_i.h"
#include "net_dq_acceptor_i.h"
#include "net_dq_endpoint.h"
#include "net_dq_dgram.h"
#include "net_dq_watcher.h"
#include "net_dq_timer.h"

static int net_dq_driver_init(net_driver_t driver);
static void net_dq_driver_fini(net_driver_t driver);

net_dq_driver_t
net_dq_driver_create(net_schedule_t schedule) {
    net_driver_t base_driver;

    base_driver = net_driver_create(
        schedule,
        "apple_dq",
        /*driver*/
        sizeof(struct net_dq_driver),
        net_dq_driver_init,
        net_dq_driver_fini,
        /*timer*/
        sizeof(struct net_dq_timer),
        net_dq_timer_init,
        net_dq_timer_fini,
        net_dq_timer_active,
        net_dq_timer_cancel,
        net_dq_timer_is_active,
        /*acceptor*/
        sizeof(struct net_dq_acceptor),
        net_dq_acceptor_init,
        net_dq_acceptor_fini,
        /*endpoint*/
        sizeof(struct net_dq_endpoint),
        net_dq_endpoint_init,
        net_dq_endpoint_fini,
        net_dq_endpoint_connect,
        net_dq_endpoint_close,
        net_dq_endpoint_update,
        /*dgram*/
        sizeof(struct net_dq_dgram),
        net_dq_dgram_init,
        net_dq_dgram_fini,
        net_dq_dgram_send,
        /*watcher*/
        sizeof(struct net_dq_watcher),
        net_dq_watcher_init,
        net_dq_watcher_fini,
        net_dq_watcher_update);


    if (base_driver == NULL) return NULL;

    net_dq_driver_t driver = net_driver_data(base_driver);

    return driver;
}

static int net_dq_driver_init(net_driver_t base_driver) {
    net_dq_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_data_monitor_fun = NULL;
    driver->m_data_monitor_ctx = NULL;
    
    return 0;
}

static void net_dq_driver_fini(net_driver_t base_driver) {
}

void net_dq_driver_free(net_dq_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

void net_dq_driver_set_data_monitor(
    net_dq_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx)
{
    driver->m_data_monitor_fun = monitor_fun;
    driver->m_data_monitor_ctx = monitor_ctx;
}

net_schedule_t net_dq_driver_schedule(net_dq_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

mem_buffer_t net_dq_driver_tmp_buffer(net_dq_driver_t driver) {
    return net_schedule_tmp_buffer(net_dq_driver_schedule(driver));
}
