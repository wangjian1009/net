#include "net_driver.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_xkcp_client_i.h"
#include "net_xkcp_endpoint_i.h"

net_xkcp_client_t
net_xkcp_client_create(net_xkcp_acceptor_t acceptor, net_address_t remote_address) {
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(net_acceptor_from_data(acceptor)));

    net_xkcp_client_t client = mem_alloc(driver->m_alloc, sizeof(struct net_xkcp_client));
    if (client == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: client %s: create: alloc fail",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        return NULL;
    }

    client->m_acceptor = acceptor;

    client->m_remote_address = net_address_copy(net_xkcp_driver_schedule(driver), remote_address);
    if (client->m_remote_address == NULL) {
        CPE_ERROR(
            driver->m_em, "xkcp: client %s: create: dup address failed",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        mem_free(driver->m_alloc, client);
        return NULL;
    }

    if (cpe_hash_table_init(
            &client->m_streams,
            driver->m_alloc,
            (cpe_hash_fun_t) net_xkcp_endpoint_hash,
            (cpe_hash_eq_t) net_xkcp_endpoint_eq,
            CPE_HASH_OBJ2ENTRY(net_xkcp_endpoint, m_svr.m_hh_for_client),
            -1) != 0)
    {
        CPE_ERROR(
            driver->m_em, "xkcp: client %s: create: init hash table fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        net_address_free(client->m_remote_address);
        mem_free(driver->m_alloc, client);
        return NULL;
    }

    cpe_hash_entry_init(&client->m_hh_for_acceptor);
    if (cpe_hash_table_insert_unique(&acceptor->m_clients, client) != 0) {
        CPE_ERROR(
            driver->m_em, "xkcp: client %s: create: insert fail!",
            net_address_dump(net_xkcp_driver_tmp_buffer(driver), remote_address));
        cpe_hash_table_fini(&client->m_streams);
        net_address_free(client->m_remote_address);
        mem_free(driver->m_alloc, client);
        return NULL;
    }
    
    return client;
}

void net_xkcp_client_free(net_xkcp_client_t client) {
    net_xkcp_acceptor_t acceptor = client->m_acceptor;
    net_xkcp_driver_t driver = net_driver_data(net_acceptor_driver(net_acceptor_from_data(acceptor)));

    struct cpe_hash_it endpoint_it;
    net_xkcp_endpoint_t endpoint;
    cpe_hash_it_init(&endpoint_it, &client->m_streams);
    endpoint = cpe_hash_it_next(&endpoint_it);
    while (endpoint) {
        net_xkcp_endpoint_t next = cpe_hash_it_next(&endpoint_it);
        if (net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_deleting);
            
        }
        endpoint = next;
    }
    cpe_hash_table_fini(&client->m_streams);
}

int net_xkcp_client_eq(net_xkcp_client_t l, net_xkcp_client_t r, void * user_data) {
    return net_address_cmp(l->m_remote_address, r->m_remote_address) == 0 ? 1 : 0;
}

uint32_t net_xkcp_client_hash(net_xkcp_client_t o, void * user_data) {
    return net_address_hash(o->m_remote_address);
}
