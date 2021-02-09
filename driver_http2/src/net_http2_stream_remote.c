#include "net_driver.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_http2_stream_remote_i.h"
#include "net_http2_endpoint_i.h"

net_http2_stream_remote_t
net_http2_stream_remote_create(net_http2_stream_driver_t driver, net_address_t address) {
    net_driver_t base_driver = net_driver_from_data(driver);
    net_schedule_t schedule = net_driver_schedule(base_driver);

    net_http2_stream_remote_t remote =
        mem_alloc(driver->m_alloc, sizeof(struct net_http2_stream_remote));
    if (driver == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: remote %s: alloc fail",
            net_driver_name(base_driver),
            net_address_dump(net_http2_stream_driver_tmp_buffer(driver), address));
        return NULL;
    }

    remote->m_driver = driver;

    remote->m_address = net_address_copy(schedule, address);
    if (remote->m_address == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: remote %s: copy address fail",
            net_driver_name(base_driver),
            net_address_dump(net_http2_stream_driver_tmp_buffer(driver), address));
        mem_free(driver->m_alloc, remote);
        return NULL;
    }
        
    cpe_hash_entry_init(&remote->m_hh_for_driver);
    if (cpe_hash_table_insert_unique(&driver->m_remotes, remote) != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: remote %s: insert fail",
            net_driver_name(base_driver),
            net_address_dump(net_http2_stream_driver_tmp_buffer(driver), address));
        net_address_free(remote->m_address);
        mem_free(driver->m_alloc, remote);
        return NULL;
    }

    TAILQ_INIT(&remote->m_endpoints);

    return remote;
}

void net_http2_stream_remote_free(net_http2_stream_remote_t remote) {
    net_http2_stream_driver_t driver = remote->m_driver;

    while(!TAILQ_EMPTY(&remote->m_endpoints)) {
        net_http2_endpoint_t endpoint = TAILQ_FIRST(&remote->m_endpoints);
        net_http2_endpoint_set_stream_remote(endpoint, NULL);
    }
    
    cpe_hash_table_remove_by_ins(&driver->m_remotes, remote);

    net_address_free(remote->m_address);
    remote->m_address = NULL;

    mem_free(driver->m_alloc, remote);
}

net_http2_stream_remote_t
net_http2_stream_remote_check_create(net_http2_stream_driver_t driver, net_address_t address) {
    net_http2_stream_remote_t remote = net_http2_stream_remote_find(driver, address);
    return remote ? remote : net_http2_stream_remote_create(driver, address);
}

net_http2_stream_remote_t
net_http2_stream_remote_find(net_http2_stream_driver_t driver, net_address_t address) {
    struct net_http2_stream_remote key;
    key.m_address = address;
    return cpe_hash_table_find(&driver->m_remotes, &key);
}

int net_http2_stream_remote_eq(net_http2_stream_remote_t l, net_http2_stream_remote_t r, void * user_data) {
    return net_address_cmp_without_port(l->m_address, r->m_address) == 0 ? 1 : 0;
}

uint32_t net_http2_stream_remote_hash(net_http2_stream_remote_t o, void * user_data) {
    return net_address_hash(o->m_address);
}
