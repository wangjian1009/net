#include "test_net_endpoint.h"

int test_net_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return 0;
}

void test_net_endpoint_fini(net_endpoint_t base_endpoint) {
}

int test_net_endpoint_connect(net_endpoint_t base_endpoint) {
    return 0;
}

void test_net_endpoint_close(net_endpoint_t base_endpoint) {
}

int test_net_endpoint_update(net_endpoint_t base_endpoint) {
    return 0;
}

int test_net_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable) {
    return 0;
}

int test_net_endpoint_get_mss(net_endpoint_t endpoint, uint32_t *mss) {
    return 128;
}

