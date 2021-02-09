#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_http2_stream_driver_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_acceptor_i.h"
#include "net_http2_stream_remote_i.h"
#include "net_http2_protocol_i.h"

static int net_http2_stream_driver_init(net_driver_t driver);
static void net_http2_stream_driver_fini(net_driver_t driver);

net_http2_stream_driver_t
net_http2_stream_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    net_http2_protocol_t underline_protocol = net_http2_protocol_find(schedule, addition_name);
    if (underline_protocol == NULL) {
        underline_protocol = net_http2_protocol_create(schedule, addition_name, alloc, em);
        if (underline_protocol == NULL) {
            return NULL;
        }
    }
    
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_http2_stream_driver),
            net_http2_stream_driver_init,
            net_http2_stream_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_http2_stream_acceptor),
            net_http2_stream_acceptor_init,
            net_http2_stream_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_http2_stream_endpoint),
            net_http2_stream_endpoint_init,
            net_http2_stream_endpoint_fini,
            net_http2_stream_endpoint_connect,
            net_http2_stream_endpoint_update,
            net_http2_stream_endpoint_set_no_delay,
            net_http2_stream_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_http2_stream_driver_t ws_driver = net_driver_data(base_driver);

    ws_driver->m_alloc = alloc;
    ws_driver->m_em = em;
    ws_driver->m_underline_driver = underline_driver;
    ws_driver->m_underline_protocol = net_protocol_from_data(underline_protocol);

    return ws_driver;
}

void net_http2_stream_driver_free(net_http2_stream_driver_t cli_driver) {
    net_driver_free(net_driver_from_data(cli_driver));
}

net_http2_stream_driver_t
net_http2_stream_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ws-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ws");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_http2_stream_driver_init(net_driver_t base_driver) {
    net_http2_stream_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    driver->m_underline_protocol = NULL;

    if (cpe_hash_table_init(
            &driver->m_remotes,
            driver->m_alloc,
            (cpe_hash_fun_t) net_http2_stream_remote_hash,
            (cpe_hash_eq_t) net_http2_stream_remote_eq,
            CPE_HASH_OBJ2ENTRY(net_http2_stream_remote, m_hh_for_driver),
            -1) != 0)
    {
        CPE_ERROR(driver->m_em, "http2: init hash table fail!");
        return -1;
    }
    
    return 0;
}

static void net_http2_stream_driver_fini(net_driver_t base_driver) {
    net_http2_stream_driver_t driver = net_driver_data(base_driver);
}

mem_buffer_t net_http2_stream_driver_tmp_buffer(net_http2_stream_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
