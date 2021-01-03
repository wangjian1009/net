#ifndef TEST_UTILS_NET_ENDPOINT_LINK_H_INCLEDED
#define TEST_UTILS_NET_ENDPOINT_LINK_H_INCLEDED
#include "test_net_endpoint.h"

struct test_net_endpoint_link {
    test_net_endpoint_t m_a;
    test_net_endpoint_t m_z;
};

test_net_endpoint_link_t test_net_endpoint_link_create(test_net_endpoint_t a, test_net_endpoint_t b);
void test_net_endpoint_link_free(test_net_endpoint_link_t link);

test_net_endpoint_t test_net_endpoint_link_other(test_net_endpoint_link_t link, test_net_endpoint_t ep);

#endif
