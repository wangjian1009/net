#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dgram.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_xkcp_config.h"
#include "net_xkcp_connector_i.h"
#include "net_xkcp_endpoint_i.h"
#include "net_xkcp_utils.h"

static void net_xkcp_connector_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_xkcp_connector_t
net_xkcp_connector_create(
    net_xkcp_driver_t driver, net_address_t remote_address, net_xkcp_config_t config)
{
    net_xkcp_connector_t connector = mem_alloc(driver->m_alloc, sizeof(struct net_xkcp_connector));
    if (connector == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: connector %s: create: alloc fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        return NULL;
    }

    connector->m_driver = driver;
    connector->m_max_conv = 0;
    connector->m_config = NULL;

    if (config) {
        connector->m_config = mem_alloc(driver->m_alloc, sizeof(struct net_xkcp_config));
        if (connector->m_config == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: connector %s: create: alloc config fail!",
                net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
            mem_free(driver->m_alloc, connector);
            return NULL;
        }

        *connector->m_config = *config;
    }
    
    connector->m_remote_address = net_address_copy(net_xkcp_driver_schedule(driver), remote_address);
    if (connector->m_remote_address == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: connector %s: create: dup address fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        if (connector->m_config) mem_free(driver->m_alloc, connector->m_config);
        mem_free(driver->m_alloc, connector);
        return NULL;
    }

    connector->m_dgram = net_dgram_create(
        driver->m_underline_driver, NULL, net_xkcp_connector_recv, connector);
    if (connector->m_dgram == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: connector %s: create: create dgram fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        net_address_free(connector->m_remote_address);
        if (connector->m_config) mem_free(driver->m_alloc, connector->m_config);
        mem_free(driver->m_alloc, connector);
        return NULL;
    }

    if (cpe_hash_table_init(
            &connector->m_streams,
            driver->m_alloc,
            (cpe_hash_fun_t) net_xkcp_endpoint_hash,
            (cpe_hash_eq_t) net_xkcp_endpoint_eq,
            CPE_HASH_OBJ2ENTRY(net_xkcp_endpoint, m_cli.m_hh_for_connector),
            -1) != 0)
    {
        CPE_ERROR(
            driver->m_em, "xkcp: connector %s: create: init hash table fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        net_dgram_free(connector->m_dgram);
        net_address_free(connector->m_remote_address);
        if (connector->m_config) mem_free(driver->m_alloc, connector->m_config);
        mem_free(driver->m_alloc, connector);
        return NULL;
    }
    
    cpe_hash_entry_init(&connector->m_hh_for_driver);
    if (cpe_hash_table_insert_unique(&driver->m_connectors, connector) != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: connector %s: create: duplicate!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        cpe_hash_table_fini(&connector->m_streams);
        net_dgram_free(connector->m_dgram);
        net_address_free(connector->m_remote_address);
        if (connector->m_config) mem_free(driver->m_alloc, connector->m_config);
        mem_free(driver->m_alloc, connector);
        return NULL;
    }
    
    return connector;
}

void net_xkcp_connector_free(net_xkcp_connector_t connector) {
    net_xkcp_driver_t driver = connector->m_driver;

    struct cpe_hash_it endpoint_it;
    net_xkcp_endpoint_t endpoint;
    cpe_hash_it_init(&endpoint_it, &connector->m_streams);
    endpoint = cpe_hash_it_next(&endpoint_it);
    while (endpoint) {
        net_xkcp_endpoint_t next = cpe_hash_it_next(&endpoint_it);
        if (net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_deleting);
            
        }
        endpoint = next;
    }
    cpe_hash_table_fini(&connector->m_streams);
    
    if (connector->m_dgram) {
        net_dgram_free(connector->m_dgram);
        connector->m_dgram = NULL;
    }

    if (connector->m_remote_address) {
        net_address_free(connector->m_remote_address);
        connector->m_remote_address = NULL;
    }

    if (connector->m_config) {
        mem_free(driver->m_alloc, connector->m_config);
        connector->m_config = NULL;
    }
    
    cpe_hash_table_remove_by_ins(&driver->m_connectors, connector);
    mem_free(driver->m_alloc, connector);
}

net_xkcp_connector_t
net_xkcp_connector_find(net_xkcp_driver_t driver, net_address_t remote_address) {
    struct net_xkcp_connector key;
    key.m_remote_address = remote_address;
    return cpe_hash_table_find(&driver->m_connectors, &key);
}

net_dgram_t net_xkcp_connector_dgram(net_xkcp_connector_t connector) {
    return connector->m_dgram;
}

net_xkcp_endpoint_t
net_xkcp_connector_find_stream(net_xkcp_connector_t connector, uint32_t conv) {
    struct net_xkcp_endpoint key;
    key.m_conv = conv;
    return cpe_hash_table_find(&connector->m_streams, &key);
}

int net_xkcp_connector_eq(net_xkcp_connector_t l, net_xkcp_connector_t r, void * user_data) {
    return net_address_cmp(l->m_remote_address, r->m_remote_address) == 0 ? 1 : 0;
}

uint32_t net_xkcp_connector_hash(net_xkcp_connector_t o, void * user_data) {
    return net_address_hash(o->m_remote_address);
}

static void net_xkcp_connector_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
    net_xkcp_connector_t connector = ctx;
    net_xkcp_driver_t driver = connector->m_driver;
    net_driver_t base_driver = net_driver_from_data(driver);

    uint32_t conv = ikcp_getconv(data);

    net_xkcp_endpoint_t endpoint = net_xkcp_connector_find_stream(connector, conv);
    if (endpoint == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: %d: <== no stream",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), source), conv);
        return;
    }

    if (endpoint->m_kcp == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: %d: <== %d data: no bind kcp",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), source), conv, (int)data_size);
        return;
    }

    int nret = ikcp_input(endpoint->m_kcp, data, data_size);
    if (nret < 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s: %d: <== %d data: ikcp_input failed [%d]",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), source), conv, (int)data_size, nret);
        return;
    }

    if (net_driver_debug(base_driver) >= 2) {
        char address_buf[128];
        cpe_str_dup(
            address_buf, sizeof(address_buf),
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), source));
        CPE_INFO(
            driver->m_em, "xkcp: %s: %d: <== %s",
            address_buf, conv,
            net_xkcp_dump_frame(net_xkcp_driver_tmp_buffer(driver), data, data_size));
    }

    net_xkcp_endpoint_kcp_forward_data(endpoint);

    if (net_endpoint_is_active(endpoint->m_base_endpoint)) {
        net_xkcp_endpoint_kcp_schedule_update(endpoint);
    }
}
