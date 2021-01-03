#ifndef TEST_UTILS_NET_ACCEPTOR_H_INCLEDED
#define TEST_UTILS_NET_ACCEPTOR_H_INCLEDED
#include "test_net_driver.h"

struct test_net_acceptor {
    TAILQ_ENTRY(test_net_acceptor) m_next;
};

int test_net_acceptor_init(net_acceptor_t base_acceptor);
void test_net_acceptor_fini(net_acceptor_t base_acceptor);

int net_sock_acceptor_accept(net_acceptor_t base_acceptor, net_endpoint_t remote_ep);

#endif
