#include <assert.h>
#include "cmocka_all.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_schedule.h"
#include "test_net_endpoint.h"
#include "test_net_endpoint_link.h"
#include "test_net_tl_op.h"

int test_net_endpoint_init(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    TAILQ_INSERT_TAIL(&driver->m_endpoints, endpoint, m_next);
    endpoint->m_write_policy.m_type = test_net_endpoint_write_mock;
    
    return 0;
}

void test_net_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    test_net_endpoint_write_policy_clear(endpoint);
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

    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
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
    int rv = mock_type(int);
    if (rv >= 0) {
        *mss = (uint32_t)rv;
        return 0;
    }
    else {
        return -1;
    }
}

int test_net_driver_read_from_other(net_endpoint_t base_endpoint, net_endpoint_t from, net_endpoint_buf_type_t buf_type) {
    if (net_endpoint_buf_append_from_other(base_endpoint, net_ep_buf_read, from, buf_type, 0) != 0) {
        if (net_endpoint_is_active(base_endpoint)) {
            if (!net_endpoint_have_error(base_endpoint)) {
                net_endpoint_set_error(
                    base_endpoint,
                    net_endpoint_error_source_network,
                    net_endpoint_network_errno_logic, NULL);
            }

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }
        }

        return -1;
    }

    return 0;
}

void test_net_endpoint_id_expect_set_no_delay(test_net_driver_t driver, uint32_t ep_id, uint8_t is_enable) {
    if (ep_id == 0) {
        expect_any(test_net_endpoint_set_no_delay, id);
    }
    else {
        expect_value(test_net_endpoint_set_no_delay, id, ep_id);
    }
    expect_value(test_net_endpoint_set_no_delay, is_enable, is_enable);
    will_return(test_net_endpoint_set_no_delay, 0);
}

void test_net_next_endpoint_expect_set_no_delay(test_net_driver_t driver, uint8_t is_enable) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_endpoint_id_expect_set_no_delay(driver, net_schedule_next_endpoint_id(schedule), is_enable);
}

void test_net_endpoint_expect_set_no_delay(net_endpoint_t base_endpoint, uint8_t is_enable) {
    test_net_endpoint_id_expect_set_no_delay(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint), is_enable);
}

void test_net_endpoint_id_expect_get_mss(test_net_driver_t driver, uint32_t ep_id, uint32_t mss) {
    expect_value(test_net_endpoint_get_mss, id, ep_id);
    will_return(test_net_endpoint_get_mss, mss);
}

void test_net_next_endpoint_expect_get_mss(test_net_driver_t driver, uint32_t mss) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_endpoint_id_expect_get_mss(driver, net_schedule_next_endpoint_id(schedule), mss);
}

void test_net_endpoint_expect_get_mss(net_endpoint_t base_endpoint, uint32_t mss) {
    test_net_endpoint_id_expect_get_mss(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint), mss);
}

void test_net_endpoint_id_expect_close(test_net_driver_t driver, uint32_t ep_id) {
    expect_value(test_net_endpoint_close, id, ep_id);
    expect_function_call(test_net_endpoint_close);
}

void test_net_next_endpoint_expect_close(test_net_driver_t driver) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_endpoint_id_expect_close(driver, net_schedule_next_endpoint_id(schedule));
}

void test_net_endpoint_expect_close(net_endpoint_t base_endpoint) {
    test_net_endpoint_id_expect_close(
        net_driver_data(net_endpoint_driver(base_endpoint)),
        net_endpoint_id(base_endpoint));
}

net_endpoint_t
test_net_driver_find_endpoint_by_remote_addr(test_net_driver_t driver, net_address_t address) {
    test_net_endpoint_t endpoint;

    TAILQ_FOREACH(endpoint, &driver->m_endpoints, m_next) {
        net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
        if (net_address_cmp_opt(net_endpoint_remote_address(base_endpoint), address) == 0) return base_endpoint;
    }
    
    return NULL;
}

net_endpoint_t
test_net_endpoint_linked_other(test_net_driver_t driver, net_endpoint_t base_endpoint) {
    assert_true(net_endpoint_driver(base_endpoint) == net_driver_from_data(driver));

    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    assert_true(endpoint->m_write_policy.m_type == test_net_endpoint_write_link);

    test_net_endpoint_t other_ep = test_net_endpoint_link_other(endpoint->m_write_policy.m_link.m_link, endpoint);
    assert_true(other_ep != NULL);

    return net_endpoint_from_data(other_ep);
}
