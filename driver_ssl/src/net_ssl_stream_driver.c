#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_ssl_stream_driver_i.h"
#include "net_ssl_stream_endpoint_i.h"
#include "net_ssl_stream_acceptor_i.h"
#include "net_ssl_protocol_i.h"

static int net_ssl_stream_driver_init(net_driver_t driver);
static void net_ssl_stream_driver_fini(net_driver_t driver);

net_ssl_stream_driver_t
net_ssl_stream_driver_create(
    net_schedule_t schedule, const char * addition_name, net_driver_t underline_driver,
    mem_allocrator_t alloc, error_monitor_t em)
{
    net_ssl_protocol_t underline_protocol = net_ssl_protocol_find(schedule, addition_name);
    if (underline_protocol == NULL) {
        underline_protocol = net_ssl_protocol_create(schedule, addition_name, alloc, em);
        if (underline_protocol == NULL) {
            return NULL;
        }
    }
    
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl");
    }

    net_driver_t base_driver =
        net_driver_create(
            schedule,
            name,
            /*driver*/
            sizeof(struct net_ssl_stream_driver),
            net_ssl_stream_driver_init,
            net_ssl_stream_driver_fini,
            NULL,
            /*timer*/
            0, NULL, NULL, NULL, NULL, NULL,
            /*acceptor*/
            sizeof(struct net_ssl_stream_acceptor),
            net_ssl_stream_acceptor_init,
            net_ssl_stream_acceptor_fini,
            /*endpoint*/
            sizeof(struct net_ssl_stream_endpoint),
            net_ssl_stream_endpoint_init,
            net_ssl_stream_endpoint_fini,
            net_ssl_stream_endpoint_connect,
            net_ssl_stream_endpoint_update,
            net_ssl_stream_endpoint_set_no_delay,
            net_ssl_stream_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL);

    net_ssl_stream_driver_t ssl_driver = net_driver_data(base_driver);

    ssl_driver->m_alloc = alloc;
    ssl_driver->m_em = em;
    ssl_driver->m_underline_driver = underline_driver;
    ssl_driver->m_underline_protocol = net_protocol_from_data(underline_protocol);

    return ssl_driver;
}

void net_ssl_stream_driver_free(net_ssl_stream_driver_t cli_driver) {
    net_driver_free(net_driver_from_data(cli_driver));
}

net_ssl_stream_driver_t
net_ssl_stream_driver_find(net_schedule_t schedule, const char * addition_name) {
    char name[64];
    if (addition_name) {
        snprintf(name, sizeof(name), "ssl-s-%s", addition_name);
    }
    else {
        snprintf(name, sizeof(name), "ssl-s");
    }

    net_driver_t driver = net_driver_find(schedule, name);
    return driver ? net_driver_data(driver) : NULL;
}

static int net_ssl_stream_driver_init(net_driver_t base_driver) {
    net_ssl_stream_driver_t driver = net_driver_data(base_driver);
    driver->m_alloc = NULL;
    driver->m_em = NULL;
    driver->m_underline_driver = NULL;
    driver->m_underline_protocol = NULL;
    return 0;
}

static void net_ssl_stream_driver_fini(net_driver_t base_driver) {
    net_ssl_stream_driver_t driver = net_driver_data(base_driver);
}

int net_ssl_stream_driver_svr_confirm_pkey(net_ssl_stream_driver_t driver) {
    net_ssl_protocol_t protocol = net_ssl_protocol_cast(driver->m_underline_protocol);
    return net_ssl_protocol_svr_confirm_pkey(protocol);
}

int net_ssl_stream_driver_svr_confirm_cert(net_ssl_stream_driver_t driver) {
    net_ssl_protocol_t protocol = net_ssl_protocol_cast(driver->m_underline_protocol);
    return net_ssl_protocol_svr_confirm_cert(protocol);
}

int net_ssl_stream_driver_svr_prepaired(net_ssl_stream_driver_t driver) {
    net_ssl_protocol_t protocol = net_ssl_protocol_cast(driver->m_underline_protocol);
    return net_ssl_protocol_svr_prepaired(protocol);
}

mem_buffer_t net_ssl_stream_driver_tmp_buffer(net_ssl_stream_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
