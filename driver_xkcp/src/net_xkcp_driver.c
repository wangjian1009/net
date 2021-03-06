#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_xkcp_driver_i.h"
#include "net_xkcp_endpoint_i.h"
#include "net_xkcp_acceptor_i.h"
#include "net_xkcp_connector_i.h"

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
            net_xkcp_endpoint_calc_size,
            net_xkcp_endpoint_connect,
            net_xkcp_endpoint_update,
            net_xkcp_endpoint_set_no_delay,
            net_xkcp_endpoint_get_mss,
            /*dgram*/
            0, NULL, NULL, NULL,
            /*watcher*/
            0, NULL, NULL, NULL,
            /*progress*/
            0, NULL, NULL);

    net_xkcp_driver_t driver = net_driver_data(base_driver);

    driver->m_alloc = alloc;
    driver->m_em = em;
    driver->m_underline_driver = underline_driver;

    if (cpe_hash_table_init(
            &driver->m_connectors,
            alloc,
            (cpe_hash_fun_t) net_xkcp_connector_hash,
            (cpe_hash_eq_t) net_xkcp_connector_eq,
            CPE_HASH_OBJ2ENTRY(net_xkcp_connector, m_hh_for_driver),
            -1) != 0)
    {
        CPE_ERROR(em, "xkcp: driver %s: create: init hash table fail!", name);
        net_driver_free(base_driver);
        return NULL;
    }
    driver->m_connections_inited = 1;

    return driver;
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
    driver->m_cfg_client_timeout_ms = 5 * 60 * 1000;
    driver->m_connections_inited = 0;
    driver->m_dft_config = NULL;

    return 0;
}

static void net_xkcp_driver_fini(net_driver_t base_driver) {
    net_xkcp_driver_t driver = net_driver_data(base_driver);

    if (driver->m_connections_inited) {
        struct cpe_hash_it connector_it;
        net_xkcp_connector_t connector;
        cpe_hash_it_init(&connector_it, &driver->m_connectors);
        connector = cpe_hash_it_next(&connector_it);
        while(connector) {
            net_xkcp_connector_t next = cpe_hash_it_next(&connector_it);
            net_xkcp_connector_free(connector);
            connector = next;
        }
        cpe_hash_table_fini(&driver->m_connectors);
    }

    if (driver->m_dft_config) {
        mem_free(driver->m_alloc, driver->m_dft_config);
        driver->m_dft_config = NULL;
    }
}

net_xkcp_driver_t net_xkcp_driver_cast(net_driver_t base_driver) {
    return net_driver_init_fun(base_driver) == net_xkcp_driver_init ? net_driver_data(base_driver) : NULL;
}

net_xkcp_config_t
net_xkcp_driver_dft_config(net_xkcp_driver_t driver) {
    return driver->m_dft_config;
}

int net_xkcp_driver_sdte_dft_config(net_xkcp_driver_t driver, net_xkcp_config_t config) {
    if (driver->m_dft_config == NULL) {
        driver->m_dft_config = mem_alloc(driver->m_alloc, sizeof(struct net_xkcp_config));
        if (driver->m_dft_config) {
            CPE_ERROR(
                driver->m_em, "xkcp: driver %s: set dft config: alloc fail!",
                net_driver_name(net_driver_from_data(driver)));
            return -1;
        }
    }

    if (!net_xkcp_config_validate(config, driver->m_em)) {
        CPE_ERROR(
            driver->m_em, "xkcp: driver %s: set dft config: validate fail!",
            net_driver_name(net_driver_from_data(driver)));
        return -1;
    }
    
    *driver->m_dft_config = *config;
    
    return 0;
}

net_schedule_t net_xkcp_driver_schedule(net_xkcp_driver_t driver) {
    return net_driver_schedule(net_driver_from_data(driver));
}

mem_buffer_t net_xkcp_driver_tmp_buffer(net_xkcp_driver_t driver) {
    return net_schedule_tmp_buffer(net_driver_schedule(net_driver_from_data(driver)));
}
