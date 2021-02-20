#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_xkcp_driver_i.h"
#include "net_xkcp_endpoint_i.h"
#include "net_xkcp_acceptor_i.h"

static int net_xkcp_driver_init(net_driver_t driver);
static void net_xkcp_driver_fini(net_driver_t driver);

net_xkcp_driver_t
net_xkcp_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "xkcp-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "xkcp");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_xkcp_driver),
            net_xkcp_driver_init,
            net_xkcp_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_xkcp_acceptor),
            net_xkcp_acceptor_init,
            net_xkcp_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_xkcp_endpoint),
            net_xkcp_endpoint_init,
            net_xkcp_endpoint_fini,
            net_xkcp_endpoint_connect,
            net_xkcp_endpoint_update,
            net_xkcp_endpoint_set_no_delay,
            net_xkcp_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_xkcp_driver_t xkcp_driver = net_driver_data(base_driver);

    xkcp_driver->m_alloc = alloc;
    xkcp_driver->m_em = em;
    xkcp_driver->m_underline_driver = underline_driver;
    
    return xkcp_driver;
}

void net_xkcp_driver_free(net_xkcp_driver_t cli_driver) {
    net_driver_free(net_driver_from_data(cli_driver));
}

net_xkcp_driver_t
net_xkcp_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "xkcp-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "xkcp");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_xkcp_driver_init(net_driver_t base_driver) {
    net_xkcp_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;

    return 0;
}

static void net_xkcp_driver_fini(net_driver_t base_driver) {
    net_xkcp_driver_t driver = net_driver_data(base_driver);
}

net_xkcp_driver_t net_xkcp_driver_cast(net_driver_t base_driver) {
    return net_driver_init_fun(base_driver) == net_xkcp_driver_init ? net_driver_data(base_driver) : NULL;
}

void net_xkcp_config_init_default(net_xkcp_config_t config) {
}

uint8_t net_xkcp_config_validate(net_xkcp_config_t config, error_monitor_t em) {
    return 0;
}

mem_buffer_t net_xkcp_driver_tmp_buffer(net_xkcp_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
