#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

int test_net_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    TAILQ_INSERT_TAIL(&driver->m_endpoints, endpoint, m_next);
    endpoint->m_op_connect = NULL;

    return 0;
}

void test_net_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_op_connect) {
        test_net_tl_op_free(endpoint->m_op_connect);
        endpoint->m_op_connect = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_endpoints, endpoint, m_next);
}

void test_net_endpoint_close(net_endpoint_t base_endpoint) {
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_op_connect) {
        test_net_tl_op_free(endpoint->m_op_connect);
        endpoint->m_op_connect = NULL;
    }
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
