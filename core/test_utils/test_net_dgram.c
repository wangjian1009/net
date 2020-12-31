#include "net_dgram.h"
#include "test_net_dgram.h"

int test_net_dgram_init(net_dgram_t base_dgram) {
    test_net_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    test_net_dgram_t dgram = net_dgram_data(base_dgram);
    TAILQ_INSERT_TAIL(&driver->m_dgrams, dgram, m_next);
    return 0;
}

void test_net_dgram_fini(net_dgram_t base_dgram) {
    test_net_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    test_net_dgram_t dgram = net_dgram_data(base_dgram);
    TAILQ_REMOVE(&driver->m_dgrams, dgram, m_next);
}

int test_net_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    return -1;
}


