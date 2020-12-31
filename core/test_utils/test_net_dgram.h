#ifndef TEST_UTILS_NET_DGRAM_H_INCLEDED
#define TEST_UTILS_NET_DGRAM_H_INCLEDED
#include "net_dgram.h"
#include "test_net_driver.h"

struct test_net_dgram {
    TAILQ_ENTRY(test_net_dgram) m_next;
};

int test_net_dgram_init(net_dgram_t base_dgram);
void test_net_dgram_fini(net_dgram_t base_dgram);
int test_net_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
