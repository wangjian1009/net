#include "net_schedule.h"
#include "net_driver.h"
#include "net_sock_driver.h"
#include "net_dq_driver_i.h"
#include "net_dq_watcher.h"
#include "net_dq_timer.h"

static int net_dq_driver_init(net_sock_driver_t sock_driver);
static void net_dq_driver_fini(net_sock_driver_t sock_driver);

net_dq_driver_t
net_dq_driver_create(net_schedule_t schedule) {
    net_sock_driver_t sock_driver;

    sock_driver = net_sock_driver_create(
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
        /*watcher*/
        sizeof(struct net_dq_watcher),
        net_dq_watcher_init,
        net_dq_watcher_fini,
        net_dq_watcher_update);


    if (sock_driver == NULL) return NULL;

    net_dq_driver_t driver = net_sock_driver_data(sock_driver);

    return driver;
}

static int net_dq_driver_init(net_sock_driver_t sock_driver) {
    net_dq_driver_t driver = net_sock_driver_data(sock_driver);
    net_schedule_t schedule = net_sock_driver_schedule(sock_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_data_monitor_fun = NULL;
    driver->m_data_monitor_ctx = NULL;
    
    return 0;
}

static void net_dq_driver_fini(net_sock_driver_t sock_driver) {
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
