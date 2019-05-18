#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_sock_driver.h"
#include "net_ev_driver_i.h"
#include "net_ev_watcher.h"
#include "net_ev_timer.h"

static int net_ev_driver_init(net_sock_driver_t driver);
static void net_ev_driver_fini(net_sock_driver_t driver);

net_ev_driver_t
net_ev_driver_create(net_schedule_t schedule, struct ev_loop * ev_loop) {
    net_sock_driver_t sock_driver;

    sock_driver = net_sock_driver_create(
        schedule,
        "ev",
        /*driver*/
        sizeof(struct net_ev_driver),
        net_ev_driver_init,
        net_ev_driver_fini,
        /*timer*/
        sizeof(struct net_ev_timer),
        net_ev_timer_init,
        net_ev_timer_fini,
        net_ev_timer_active,
        net_ev_timer_cancel,
        net_ev_timer_is_active,
        /*watcher*/
        sizeof(struct net_ev_watcher),
        net_ev_watcher_init,
        net_ev_watcher_fini,
        net_ev_watcher_update);

    if (sock_driver == NULL) return NULL;

    net_ev_driver_t driver = net_sock_driver_data(sock_driver);
    driver->m_ev_loop = ev_loop;

    return driver;
}

static int net_ev_driver_init(net_sock_driver_t base_driver) {
    net_ev_driver_t driver = net_sock_driver_data(base_driver);
    net_schedule_t schedule = net_sock_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_ev_loop = NULL;

    return 0;
}

static void net_ev_driver_fini(net_sock_driver_t base_driver) {
}

void net_ev_driver_free(net_ev_driver_t driver) {
    net_sock_driver_free(net_sock_driver_from_data(driver));
}

net_ev_driver_t net_ev_driver_cast(net_driver_t driver) {
    return strcmp(net_driver_name(driver), "ev") == 0 ? net_sock_driver_data_from_base_driver(driver) : NULL;
}

struct ev_loop * net_ev_driver_loop(net_ev_driver_t driver) {
    return driver->m_ev_loop;
}

net_schedule_t net_ev_driver_schedule(net_ev_driver_t driver) {
    return net_sock_driver_schedule(net_sock_driver_from_data(driver));
}

net_driver_t net_ev_driver_base_driver(net_ev_driver_t driver) {
    return net_driver_from_data(net_sock_driver_from_data(driver));
}

mem_buffer_t net_ev_driver_tmp_buffer(net_ev_driver_t driver) {
    return net_schedule_tmp_buffer(net_ev_driver_schedule(driver));
}
