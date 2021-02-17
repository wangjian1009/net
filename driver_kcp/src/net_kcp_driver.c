#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_kcp_driver_i.h"
#include "net_kcp_endpoint_i.h"
#include "net_kcp_acceptor_i.h"

static int net_kcp_driver_init(net_driver_t driver);
static void net_kcp_driver_fini(net_driver_t driver);

net_kcp_driver_t
net_kcp_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "kcp-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "kcp");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_kcp_driver),
            net_kcp_driver_init,
            net_kcp_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_kcp_acceptor),
            net_kcp_acceptor_init,
            net_kcp_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_kcp_endpoint),
            net_kcp_endpoint_init,
            net_kcp_endpoint_fini,
            net_kcp_endpoint_connect,
            net_kcp_endpoint_update,
            net_kcp_endpoint_set_no_delay,
            net_kcp_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_kcp_driver_t kcp_driver = net_driver_data(base_driver);

    kcp_driver->m_alloc = alloc;
    kcp_driver->m_em = em;
    kcp_driver->m_underline_driver = underline_driver;

    return kcp_driver;
}

void net_kcp_driver_free(net_kcp_driver_t cli_driver) {
    net_driver_free(net_driver_from_data(cli_driver));
}

net_kcp_driver_t
net_kcp_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "kcp-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "kcp");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_kcp_driver_init(net_driver_t base_driver) {
    net_kcp_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    return 0;
}

static void net_kcp_driver_fini(net_driver_t base_driver) {
    net_kcp_driver_t driver = net_driver_data(base_driver);
}

mem_buffer_t net_kcp_driver_tmp_buffer(net_kcp_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
