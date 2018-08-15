#include "net_schedule.h"
#include "net_driver.h"
#include "net_ne_driver_i.h"
#include "net_ne_endpoint.h"
#include "net_ne_dgram.h"
#include "net_ne_dgram_session.h"

static int net_ne_driver_init(net_driver_t driver);
static void net_ne_driver_fini(net_driver_t driver);

net_ne_driver_t
net_ne_driver_create(net_schedule_t schedule, NETunnelProvider* tunnel_provider) {
    net_driver_t base_driver;

    base_driver = net_driver_create(
        schedule,
        "apple_ne",
        /*driver*/
        sizeof(struct net_ne_driver),
        net_ne_driver_init,
        net_ne_driver_fini,
        /*timer*/
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        /*acceptor*/
        0,
        NULL,
        NULL,
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

    driver->m_tunnel_provider = tunnel_provider;
    [driver->m_tunnel_provider retain];
    
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
    driver->m_dgram_timeout_ms = 300 * 1000;
    driver->m_debug = 0;
    
    TAILQ_INIT(&driver->m_free_dgram_sessions);

    return 0;
}

static void net_ne_driver_fini(net_driver_t base_driver) {
    net_ne_driver_t driver = net_driver_data(base_driver);

    if (driver->m_tunnel_provider) {
        [driver->m_tunnel_provider release];
        driver->m_tunnel_provider = NULL;
    }

    while(!TAILQ_EMPTY(&driver->m_free_dgram_sessions)) {
        net_ne_dgram_session_real_free(TAILQ_FIRST(&driver->m_free_dgram_sessions));
    }

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

net_schedule_t net_ne_driver_schedule(net_ne_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

mem_buffer_t net_ne_driver_tmp_buffer(net_ne_driver_t driver) {
    return net_schedule_tmp_buffer(net_ne_driver_schedule(driver));
}
