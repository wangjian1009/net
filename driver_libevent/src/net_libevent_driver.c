#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_libevent_driver_i.h"
#include "net_libevent_watcher.h"
#include "net_libevent_timer.h"
#include "net_libevent_acceptor_i.h"
#include "net_libevent_endpoint.h"
#include "net_libevent_dgram.h"

static int net_libevent_driver_init(net_driver_t driver);
static void net_libevent_driver_fini(net_driver_t driver);

net_libevent_driver_t
net_libevent_driver_create(net_schedule_t schedule, struct event_base * event_base) {
    net_driver_t base_driver = net_driver_create(
        schedule,
        "libevent",
        /*driver*/
        sizeof(struct net_libevent_driver),
        net_libevent_driver_init,
        net_libevent_driver_fini,
        /*timer*/
        sizeof(struct net_libevent_timer),
        net_libevent_timer_init,
        net_libevent_timer_fini,
        net_libevent_timer_active,
        net_libevent_timer_cancel,
        net_libevent_timer_is_active,
        /*acceptor*/
        sizeof(struct net_libevent_acceptor),
        net_libevent_acceptor_init,
        net_libevent_acceptor_fini,
        /*endpoint*/
        sizeof(struct net_libevent_endpoint),
        net_libevent_endpoint_init,
        net_libevent_endpoint_fini,
        net_libevent_endpoint_connect,
        net_libevent_endpoint_close,
        net_libevent_endpoint_update,
        /*dgram*/
        sizeof(struct net_libevent_dgram),
        net_libevent_dgram_init,
        net_libevent_dgram_fini,
        net_libevent_dgram_send,
        /*watcher*/
        sizeof(struct net_libevent_watcher),
        net_libevent_watcher_init,
        net_libevent_watcher_fini,
        net_libevent_watcher_update);

    if (base_driver == NULL) return NULL;

    net_libevent_driver_t driver = net_driver_data(base_driver);
    driver->m_event_base = event_base;

    return driver;
}

static int net_libevent_driver_init(net_driver_t base_driver) {
    net_libevent_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_event_base = NULL;

    return 0;
}

static void net_libevent_driver_fini(net_driver_t base_driver) {
}

void net_libevent_driver_free(net_libevent_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

net_libevent_driver_t net_libevent_driver_cast(net_driver_t driver) {
    return strcmp(net_driver_name(driver), "libevent") == 0 ? net_driver_data(driver) : NULL;
}

struct event_base * net_libevent_driver_event_base(net_libevent_driver_t driver) {
    return driver->m_event_base;
}

net_schedule_t net_libevent_driver_schedule(net_libevent_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

net_driver_t net_libevent_driver_base_driver(net_libevent_driver_t driver) {
    return net_driver_from_data(driver);
}

mem_buffer_t net_libevent_driver_tmp_buffer(net_libevent_driver_t driver) {
    return net_schedule_tmp_buffer(net_libevent_driver_schedule(driver));
}
