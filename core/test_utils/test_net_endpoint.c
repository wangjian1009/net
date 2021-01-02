#include <assert.h>
#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

int test_net_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    TAILQ_INSERT_TAIL(&driver->m_endpoints, endpoint, m_next);
    endpoint->m_writing_op = NULL;
    
    return 0;
}

void test_net_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    TAILQ_REMOVE(&driver->m_endpoints, endpoint, m_next);
}

void test_net_endpoint_close(net_endpoint_t base_endpoint) {
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);
    function_called();
}

int test_net_endpoint_update(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert_true(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*有数据等待写入 */
        && endpoint->m_writing_op == NULL) /*socket没有等待可以写入的操作（当前可以写入数据到socket) */
    {
        net_endpoint_set_is_writing(base_endpoint, 1);
        test_net_endpoint_write(base_endpoint);
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
    }

    assert_true(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    /* if (net_endpoint_expect_read(base_endpoint)) { */
    /*     net_watcher_update_read(endpoint->m_watcher, 1); */
    /* } else { */
    /*     net_watcher_update_read(endpoint->m_watcher, 0); *\/ */
    /* } */

    return 0;
}

int test_net_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t is_enable) {
    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);
    check_expected(is_enable);
    return mock_type(int);
}

int test_net_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t *mss) {
    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);
    *mss = mock_type(uint32_t);
    return mock_type(int);
}

void test_net_driver_expect_set_no_delay(uint8_t is_enable) {
    expect_any(test_net_endpoint_set_no_delay, id);
    expect_value(test_net_endpoint_set_no_delay, is_enable, is_enable);
    will_return(test_net_endpoint_set_no_delay, 0);
}

void test_net_driver_expect_close() {
    expect_any(test_net_endpoint_close, id);
    expect_function_call(test_net_endpoint_close);
}

void test_net_endpoint_expect_close(net_endpoint_t base_endpoint) {
    uint32_t id = net_endpoint_id(base_endpoint);
    expect_value(test_net_endpoint_close, id, id);
    expect_function_call(test_net_endpoint_close);
}
