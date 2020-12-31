#ifndef TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#define TEST_UTILS_NET_ENDPOINT_H_INCLEDED
#include "net_endpoint.h"
#include "test_net_driver.h"

struct test_net_endpoint {
    TAILQ_ENTRY(test_net_endpoint) m_next;
    test_net_tl_op_t m_op_connect;
};

int test_net_endpoint_init(net_endpoint_t base_endpoint);
void test_net_endpoint_fini(net_endpoint_t base_endpoint);
int test_net_endpoint_connect(net_endpoint_t base_endpoint);
void test_net_endpoint_close(net_endpoint_t base_endpoint);
int test_net_endpoint_update(net_endpoint_t base_endpoint);
int test_net_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable);
int test_net_endpoint_get_mss(net_endpoint_t endpoint, uint32_t *mss);

#endif
