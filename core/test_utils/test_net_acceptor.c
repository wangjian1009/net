#include "net_acceptor.h"
#include "test_net_acceptor.h"

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

