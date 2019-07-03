#include "net_schedule.h"
#include "net_driver.h"
#include "net_sock_driver.h"
#include "net_android_driver_i.h"
#include "net_android_watcher.h"
#include "net_android_timer.h"

static int net_android_driver_init(net_sock_driver_t sock_driver);
static void net_android_driver_fini(net_sock_driver_t sock_driver);

net_android_driver_t
net_android_driver_create(net_schedule_t schedule) {
    net_sock_driver_t sock_driver;

    sock_driver = net_sock_driver_create(
        schedule,
        "android",
        /*driver*/
        sizeof(struct net_android_driver),
        net_android_driver_init,
        net_android_driver_fini,
        /*timer*/
        sizeof(struct net_android_timer),
        net_android_timer_init,
        net_android_timer_fini,
        net_android_timer_active,
        net_android_timer_cancel,
        net_android_timer_is_active,
        /*watcher*/
        sizeof(struct net_android_watcher),
        net_android_watcher_init,
        net_android_watcher_fini,
        net_android_watcher_update);


    if (sock_driver == NULL) return NULL;

    net_android_driver_t driver = net_sock_driver_data(sock_driver);

    return driver;
}

static int net_android_driver_init(net_sock_driver_t sock_driver) {
    net_android_driver_t driver = net_sock_driver_data(sock_driver);
    net_schedule_t schedule = net_sock_driver_schedule(sock_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    
    return 0;
}

static void net_android_driver_fini(net_sock_driver_t sock_driver) {
}

void net_android_driver_free(net_android_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

void net_android_driver_set_data_monitor(
    net_android_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx)
{
}

net_schedule_t net_android_driver_schedule(net_android_driver_t driver) {
    return net_driver_schedule(net_android_driver_base_driver(driver));
}

net_driver_t net_android_driver_base_driver(net_android_driver_t driver) {
    return net_sock_driver_base_driver(net_sock_driver_from_data(driver));
}

mem_buffer_t net_android_driver_tmp_buffer(net_android_driver_t driver) {
    return net_schedule_tmp_buffer(net_android_driver_schedule(driver));
}
