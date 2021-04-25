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

    if (net_endpoint_driver_debug(base_endpoint) < 2) {
        net_endpoint_set_driver_debug(base_endpoint, 2);
    }
    
    TAILQ_INSERT_TAIL(&driver->m_endpoints, endpoint, m_next);
    endpoint->m_write_policy.m_type = test_net_endpoint_write_mock;
    endpoint->m_set_no_delay_mock = 0;
    endpoint->m_get_mss_mock = 0;

    return 0;
}

void test_net_endpoint_fini(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    test_net_endpoint_write_policy_clear(endpoint);
    TAILQ_REMOVE(&driver->m_endpoints, endpoint, m_next);
}

int test_net_endpoint_update(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) { /*有数据等待写入 */
            net_endpoint_set_is_writing(base_endpoint, 1);
            test_net_endpoint_write(base_endpoint);
            if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
        }

        /* if (net_endpoint_expect_read(base_endpoint)) { */
        /*     net_watcher_update_read(endpoint->m_watcher, 1); */
        /* } else { */
        /*     net_watcher_update_read(endpoint->m_watcher, 0); *\/ */
        /* } */
    }

    return 0;
}

int test_net_endpoint_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    if (!endpoint->m_set_no_delay_mock) return 0;

    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);
    check_expected(no_delay);
    return mock_type(int);
}

int test_net_endpoint_get_mss(net_endpoint_t base_endpoint, uint32_t *mss) {
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    if (!endpoint->m_get_mss_mock) return 0;

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
                    net_endpoint_network_errno_internal, NULL);
            }

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            }
        }

        return -1;
    }

    return 0;
}

test_net_endpoint_t test_net_endpoint_cast(net_endpoint_t base_endpoint) {
    net_driver_t driver = net_endpoint_driver(base_endpoint);
    return net_driver_endpoint_init_fun(driver) == test_net_endpoint_init
        ? net_endpoint_data(base_endpoint)
        : NULL;
}

void test_net_endpoint_expect_set_no_delay(net_endpoint_t base_endpoint, uint8_t no_delay) {
    test_net_endpoint_t endpoint = test_net_endpoint_cast(base_endpoint);
    endpoint->m_set_no_delay_mock = 1;

    expect_value(test_net_endpoint_set_no_delay, id, net_endpoint_id(base_endpoint));
    expect_value(test_net_endpoint_set_no_delay, no_delay, no_delay);
    will_return(test_net_endpoint_set_no_delay, 0);
}

void test_net_endpoint_expect_get_mss(net_endpoint_t base_endpoint, uint32_t mss) {
    test_net_endpoint_t endpoint = test_net_endpoint_cast(base_endpoint);
    endpoint->m_get_mss_mock = 1;
    expect_value(test_net_endpoint_get_mss, id, net_endpoint_id(base_endpoint));
    will_return(test_net_endpoint_get_mss, mss);
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
    assert(endpoint->m_write_policy.m_type == test_net_endpoint_write_link);
    assert_true(endpoint->m_write_policy.m_type == test_net_endpoint_write_link);

    test_net_endpoint_t other_ep = test_net_endpoint_link_other(endpoint->m_write_policy.m_link.m_link, endpoint);
    assert_true(other_ep != NULL);

    return net_endpoint_from_data(other_ep);
}

struct test_net_endpoint_error_op {
    uint32_t m_ep_id;
    net_endpoint_error_source_t m_error_source;
    int m_error_no;
    const char * m_error_msg;
};

void test_net_endpoint_error_op_cb(void * ctx, test_net_tl_op_t op) {
    test_net_driver_t driver = op->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    struct test_net_endpoint_error_op * error_op = test_net_tl_op_data(op);

    net_endpoint_t to_endpoint = net_endpoint_find(schedule, error_op->m_ep_id);
    if (to_endpoint != NULL && net_endpoint_state(to_endpoint) != net_endpoint_state_deleting) {
        net_endpoint_set_error(to_endpoint, error_op->m_error_source, error_op->m_error_no, error_op->m_error_msg);
        if (net_endpoint_set_state(to_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(to_endpoint, net_endpoint_state_deleting);
        }
    }
}


void test_net_endpoint_error(
    test_net_driver_t driver, net_endpoint_t base_endpoint,
    net_endpoint_error_source_t error_source, int error_no, const char * error_msg,
    int64_t delay_ms)
{
    if (delay_ms == 0) {
        net_endpoint_set_error(base_endpoint, error_source, error_no, error_msg);
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        }
    }
    else {
        test_net_tl_op_t op = test_net_tl_op_create(
            driver, delay_ms,
            sizeof(struct test_net_endpoint_error_op),
            test_net_endpoint_error_op_cb, NULL, NULL);
        assert_true(op != NULL);

        struct test_net_endpoint_error_op * error_op = test_net_tl_op_data(op);
        error_op->m_ep_id = net_endpoint_id(base_endpoint);
        error_op->m_error_source = error_source;
        error_op->m_error_no = error_no;
        error_op->m_error_msg = error_msg ? mem_buffer_strdup(&driver->m_setup_buffer, error_msg) : NULL;
    }
}
