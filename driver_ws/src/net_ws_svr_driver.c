#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_ws_svr_driver_i.h"
#include "net_ws_svr_acceptor_i.h"
#include "net_ws_svr_endpoint_i.h"
#include "net_ws_svr_underline_i.h"

static int net_ws_svr_driver_init(net_driver_t driver);
static void net_ws_svr_driver_fini(net_driver_t driver);

static int net_ws_svr_driver_init_cert(
    net_driver_t base_driver, net_ws_svr_driver_t driver, const char * key, const char * cert);

net_ws_svr_driver_t
net_ws_svr_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws-svr");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_ws_svr_driver),
            net_ws_svr_driver_init,
            net_ws_svr_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_ws_svr_acceptor),
            net_ws_svr_acceptor_init,
            net_ws_svr_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_ws_svr_endpoint),
            net_ws_svr_endpoint_init,
            net_ws_svr_endpoint_fini,
            NULL,
            net_ws_svr_endpoint_update,
            net_ws_svr_endpoint_set_no_delay,
            net_ws_svr_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_ws_svr_driver_t ws_driver = net_driver_data(base_driver);

    ws_driver->m_alloc = alloc;
    ws_driver->m_em = em;
    ws_driver->m_underline_driver = underline_driver;
    ws_driver->m_underline_protocol = net_ws_svr_underline_protocol_create(schedule, name, ws_driver);

    return ws_driver;
}

void net_ws_svr_driver_free(net_ws_svr_driver_t svr_driver) {
    net_driver_free(net_driver_from_data(svr_driver));
}

net_ws_svr_driver_t
net_ws_svr_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-svr-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws-svr");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_ws_svr_driver_init(net_driver_t base_driver) {
    net_ws_svr_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    driver->m_underline_protocol = NULL;
   
    return 0;
}

static void net_ws_svr_driver_fini(net_driver_t base_driver) {
    net_ws_svr_driver_t driver = net_driver_data(base_driver);

    if (driver->m_underline_protocol) {
        net_protocol_free(driver->m_underline_protocol);
        driver->m_underline_protocol = NULL;
    }
}

mem_buffer_t net_ws_svr_driver_tmp_buffer(net_ws_svr_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
