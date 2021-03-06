#include "cmocka_all.h"
#include "net_acceptor.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "test_net_acceptor.h"
#include "test_net_endpoint.h"
#include "test_net_endpoint_link.h"

int test_net_acceptor_init(net_acceptor_t base_acceptor) {
    test_net_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    test_net_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    TAILQ_INSERT_TAIL(&driver->m_acceptors, acceptor, m_next);
    return 0;
}

void test_net_acceptor_fini(net_acceptor_t base_acceptor) {
    test_net_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    test_net_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    TAILQ_REMOVE(&driver->m_acceptors, acceptor, m_next);
}

int test_net_acceptor_accept(net_acceptor_t base_acceptor, net_endpoint_t remote_ep, int64_t write_delay_ms) {
    net_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    test_net_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    net_endpoint_t new_base_endpoint =
        net_endpoint_create(net_driver_from_data(driver), net_acceptor_protocol(base_acceptor), NULL);
    assert_true(new_base_endpoint != NULL);

    test_net_endpoint_t new_endpoint = net_endpoint_data(new_base_endpoint);

    net_endpoint_set_remote_address(new_base_endpoint, net_endpoint_address(remote_ep));

    test_net_endpoint_link_create(net_endpoint_data(remote_ep), new_endpoint, write_delay_ms);
    
    if (net_endpoint_set_state(new_base_endpoint, net_endpoint_state_established) != 0) {
        net_endpoint_free(new_base_endpoint);
        return -1;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, new_base_endpoint) != 0) {
        net_endpoint_free(new_base_endpoint);
        return -1;
    }

    return 0;
}

net_acceptor_t
test_net_driver_find_acceptor_by_addr(test_net_driver_t driver, net_address_t address) {
    test_net_acceptor_t acceptor;

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    TAILQ_FOREACH(acceptor, &driver->m_acceptors, m_next) {
        net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
        if (net_address_cmp_opt(net_acceptor_address(base_acceptor), address) == 0) return base_acceptor;
    }
    
    return NULL;
}
