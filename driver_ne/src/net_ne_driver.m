#include "net_schedule.h"
#include "net_driver.h"
#include "net_ne_driver_i.h"
#include "net_ne_acceptor_i.h"
#include "net_ne_endpoint.h"
#include "net_ne_dgram.h"
#include "net_ne_timer.h"
#include "net_ne_dgram_session.h"

static int net_ne_driver_init(net_driver_t driver);
static void net_ne_driver_fini(net_driver_t driver);

net_ne_driver_t
net_ne_driver_create(net_schedule_t schedule, NETunnelProvider* tunnel_provider) {
    net_driver_t base_driver;

    base_driver = net_driver_create(
        schedule,
        "ios_ne",
        /*driver*/
        sizeof(struct net_ne_driver),
        net_ne_driver_init,
        net_ne_driver_fini,
        /*timer*/
        sizeof(struct net_ne_timer),
        net_ne_timer_init,
        net_ne_timer_fini,
        net_ne_timer_active,
        net_ne_timer_cancel,
        net_ne_timer_is_active,
        /*endpoint*/
        sizeof(struct net_ne_endpoint),
        net_ne_endpoint_init,
        net_ne_endpoint_fini,
        net_ne_endpoint_connect,
        net_ne_endpoint_close,
        net_ne_endpoint_on_output,
        /*dgram*/
        sizeof(struct net_ne_dgram),
        net_ne_dgram_init,
        net_ne_dgram_fini,
        net_ne_dgram_send);

    if (base_driver == NULL) return NULL;

    net_ne_driver_t driver = net_driver_data(base_driver);
    driver->m_tunnel_provider = (__bridge NETunnelProvider*)tunnel_provider;

    return net_driver_data(base_driver);
}

static int net_ne_driver_init(net_driver_t base_driver) {
    net_ne_driver_t driver = net_driver_data(base_driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    driver->m_alloc = net_schedule_allocrator(schedule);
    driver->m_em = net_schedule_em(schedule);
    driver->m_tunnel_provider = NULL;
    driver->m_data_monitor_fun = NULL;
    driver->m_data_monitor_ctx = NULL;
    driver->m_debug = 0;

    driver->m_queue = dispatch_queue_create("net_driver_ne", nil);
    if (driver->m_queue == NULL) {
        CPE_ERROR(driver->m_em, "ne: create dispatch queue fail!");
        return -1;
    }
    [driver->m_queue retain];

    TAILQ_INIT(&driver->m_acceptors);
    TAILQ_INIT(&driver->m_free_dgram_sessions);

    return 0;
}

static void net_ne_driver_fini(net_driver_t base_driver) {
    net_ne_driver_t driver = net_driver_data(base_driver);

    while(!TAILQ_EMPTY(&driver->m_acceptors)) {
        net_ne_acceptor_free(TAILQ_FIRST(&driver->m_acceptors));
    }

    if (driver->m_tunnel_provider) {
        [driver->m_tunnel_provider release];
        driver->m_tunnel_provider = NULL;
    }

    while(!TAILQ_EMPTY(&driver->m_free_dgram_sessions)) {
        net_ne_dgram_session_real_free(TAILQ_FIRST(&driver->m_free_dgram_sessions));
    }

    [driver->m_queue release];
}

void net_ne_driver_free(net_ne_driver_t driver) {
    net_driver_free(net_driver_from_data(driver));
}

uint8_t net_ne_driver_debug(net_ne_driver_t driver) {
    return driver->m_debug;
}

void net_ne_driver_set_debug(net_ne_driver_t driver, uint8_t debug) {
    driver->m_debug = debug;
}

void net_ne_driver_set_data_monitor(
    net_ne_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx)
{
    driver->m_data_monitor_fun = monitor_fun;
    driver->m_data_monitor_ctx = monitor_ctx;
}

mem_buffer_t net_ne_driver_tmp_buffer(net_ne_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
