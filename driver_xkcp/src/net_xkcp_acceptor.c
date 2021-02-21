#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_protocol.h"
#include "net_acceptor.h"
#include "net_endpoint.h"
#include "net_xkcp_acceptor_i.h"
#include "net_xkcp_client_i.h"
#include "net_xkcp_endpoint_i.h"
#include "net_xkcp_utils.h"

static void net_xkcp_acceptor_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

int net_xkcp_acceptor_init(net_acceptor_t base_acceptor) {
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    acceptor->m_config = NULL;

    net_address_t address = net_acceptor_address(base_acceptor);
    if (address == NULL) {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: no address!");
        return -1;
    }

    acceptor->m_dgram = net_dgram_create(driver->m_underline_driver, address, net_xkcp_acceptor_recv, base_acceptor);
    if (acceptor->m_dgram == NULL) {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: create dgram fail!");
        return -1;
    }

    if (cpe_hash_table_init(
            &acceptor->m_clients,
            driver->m_alloc,
            (cpe_hash_fun_t) net_xkcp_client_hash,
            (cpe_hash_eq_t) net_xkcp_client_eq,
            CPE_HASH_OBJ2ENTRY(net_xkcp_client, m_hh_for_acceptor),
            -1) != 0)
    {
        CPE_ERROR(driver->m_em, "xkcp: acceptor: init: init client hash table fail!");
        net_dgram_free(acceptor->m_dgram);
        return -1;
    }
    
    return 0;
}

void net_xkcp_acceptor_fini(net_acceptor_t base_acceptor) {
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    struct cpe_hash_it client_it;
    net_xkcp_client_t client;
    cpe_hash_it_init(&client_it, &acceptor->m_clients);
    client = cpe_hash_it_next(&client_it);
    while(client) {
        net_xkcp_client_t next = cpe_hash_it_next(&client_it);
        net_xkcp_client_free(client);
        client = next;
    }
    cpe_hash_table_fini(&acceptor->m_clients);
    
    if (acceptor->m_dgram) {
        net_dgram_free(acceptor->m_dgram);
        acceptor->m_dgram = NULL;
    }

    if (acceptor->m_config) {
        mem_free(driver->m_alloc, acceptor->m_config);
        acceptor->m_config = NULL;
    }
}

int net_xkcp_acceptor_set_config(net_xkcp_acceptor_t acceptor, net_xkcp_config_t config) {
    net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    if (net_xkcp_config_validate(config, driver->m_em) != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: acceptor %s: set config: config error!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), net_acceptor_address(base_acceptor)));
        return -1;
    }

    if (acceptor->m_config == NULL) {
        acceptor->m_config = mem_alloc(driver->m_alloc, sizeof(struct net_xkcp_config));
        if (acceptor->m_config == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: acceptor %s: set config: alloc config fail!",
                net_address_dump(net_xkcp_driver_tmp_buffer(driver), net_acceptor_address(base_acceptor)));
            return -1;
        }
    }
    
    *acceptor->m_config = *config;

    return 0;
}

net_dgram_t net_xkcp_acceptor_dgram(net_xkcp_acceptor_t acceptor) {
    return acceptor->m_dgram;
}

net_xkcp_client_t
net_xkcp_acceptor_find_client(net_xkcp_acceptor_t acceptor, net_address_t remote_address) {
    struct net_xkcp_client key;
    key.m_remote_address = remote_address;
    return cpe_hash_table_find(&acceptor->m_clients, &key);
}

static void net_xkcp_acceptor_recv(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
    net_acceptor_t base_acceptor = ctx;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_xkcp_driver_t driver = net_driver_data(base_driver);
    net_xkcp_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    if (source == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s source address",
            net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0));
        return;
    }
    
    uint32_t conv = ikcp_getconv(data);

    net_xkcp_endpoint_t endpoint = NULL;

    net_xkcp_client_t client = net_xkcp_acceptor_find_client(acceptor, source);
    if (client) {
        endpoint = net_xkcp_client_find_stream(client, conv);
    }
    else {
        client = net_xkcp_client_create(acceptor, source);
        if (client == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s conv=%d: create client error",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
            return;
        }
    }

    if (endpoint == NULL) {
        net_endpoint_t base_endpoint =
            net_endpoint_create(
                net_driver_from_data(driver),
                net_acceptor_protocol(base_acceptor),
                NULL);
        if (base_endpoint == NULL) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s conv=%d: new endpoint failed",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
            return;
        }

        endpoint = net_xkcp_endpoint_cast(base_endpoint);
        net_xkcp_endpoint_set_runing_mode(endpoint, net_xkcp_endpoint_runing_mode_svr);

        if (net_xkcp_endpoint_set_conv(endpoint, conv) != 0) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s conv=%d: new endpoint: set conv failed",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
            net_endpoint_free(base_endpoint);
            return;
        }

        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s conv=%d: new endpoint: set established fail",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
            net_endpoint_free(base_endpoint);
            return;
        }

        if (net_acceptor_on_new_endpoint(base_acceptor, base_endpoint) != 0) {
            CPE_ERROR(
                driver->m_em, "xkcp: %s conv=%d: new endpoint: accept fail",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
            net_endpoint_free(base_endpoint);
            return;
        }

        if (net_driver_debug(base_driver) >= 2) {
            CPE_INFO(
                driver->m_em, "xkcp: %s conv=%d: new endpoint",
                net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
                conv);
        }
    }

    if (endpoint->m_kcp == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s conv=%d: no bind kcp",
            net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
            conv);
        return;
    }

    int nret = ikcp_input(endpoint->m_kcp, data, data_size);
    if (nret < 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: %s conv=%d: ikcp_input %d data failed [%d]",
            net_xkcp_dump_address_pair(net_xkcp_driver_tmp_buffer(driver), net_dgram_address(dgram), source, 0),
            conv, (int)data_size, nret);
        return;
    }

    if (net_driver_debug(base_driver) >= 2) {
        CPE_INFO(
            driver->m_em, "xkcp: %s",
            net_xkcp_dump_frame(
                net_xkcp_driver_tmp_buffer(driver),
                net_dgram_address(dgram), source, 0, data, data_size));
    }

    net_xkcp_endpoint_kcp_forward_data(endpoint);

    if (net_endpoint_is_active(endpoint->m_base_endpoint)) {
        net_xkcp_endpoint_kcp_schedule_update(endpoint);
    }
}
