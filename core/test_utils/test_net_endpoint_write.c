#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_endpoint_link.h"
#include "test_net_tl_op.h"

struct test_net_endpoint_write_setup {
    struct test_net_endpoint_write_policy m_write_policy;
    void * m_expect_buf;
    uint32_t m_expect_size;
};

void test_net_endpoint_write_policy_clear(test_net_endpoint_t endpoint) {
    switch(endpoint->m_write_policy.m_type) {
    case test_net_endpoint_write_mock:
    case test_net_endpoint_write_noop:
    case test_net_endpoint_write_keep:
    case test_net_endpoint_write_remove:
    case test_net_endpoint_write_error:
        break;
    case test_net_endpoint_write_link:
        break;
    }

    endpoint->m_write_policy.m_type = test_net_endpoint_write_mock;
}

struct test_net_endpoint_write_op {
    uint32_t m_ep_id;
    uint32_t m_data_size;
};

void test_net_endpoint_write_op_cb(void * ctx, test_net_tl_op_t op) {
    test_net_driver_t driver = op->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    struct test_net_endpoint_write_op * write_op = test_net_tl_op_data(op);

    net_endpoint_t to_endpoint = net_endpoint_find(schedule, write_op->m_ep_id);
    if (to_endpoint != NULL && net_endpoint_state(to_endpoint) != net_endpoint_state_deleting) {
        if (net_endpoint_driver_debug(to_endpoint) >= 2) {
            CPE_INFO(
                net_schedule_em(schedule), "test: %s: << %d data",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), to_endpoint),
                write_op->m_data_size);
        }
        net_endpoint_buf_append(to_endpoint, net_ep_buf_read, write_op + 1, write_op->m_data_size);
    }
}

void test_net_endpoint_write_delay(
    test_net_driver_t driver, net_endpoint_t from_endpoint, net_endpoint_t to_endpoint, int64_t delay_ms)
{
    uint32_t data_size = net_endpoint_buf_size(from_endpoint, net_ep_buf_write);
    if (data_size == 0) return;

    void * data = NULL;
    assert_true(net_endpoint_buf_peak_with_size(from_endpoint, net_ep_buf_write, data_size, &data) == 0);
    
    test_net_tl_op_t op =
        test_net_tl_op_create(
            driver, delay_ms,
            sizeof(struct test_net_endpoint_write_op) + data_size,
            test_net_endpoint_write_op_cb, NULL, NULL);
    assert_true(op != NULL);
    
    struct test_net_endpoint_write_op * write_op = test_net_tl_op_data(op);
    write_op->m_ep_id = net_endpoint_id(to_endpoint);
    write_op->m_data_size = data_size;
    memcpy(write_op + 1, data, data_size);

    net_endpoint_buf_consume(from_endpoint, net_ep_buf_write, data_size);
}

void test_net_endpoint_write(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

PROCESS_AGAIN:
    if (endpoint->m_write_policy.m_type == test_net_endpoint_write_mock) {
        uint32_t id = net_endpoint_id(base_endpoint);
        check_expected(id);

        struct test_net_endpoint_write_setup * setup = mock_type(struct test_net_endpoint_write_setup *);
        assert_ptr_not_equal(setup, NULL);

        endpoint->m_write_policy = setup->m_write_policy;

        if (endpoint->m_write_policy.m_type == test_net_endpoint_write_mock) {
            uint32_t buf_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
            assert_true(buf_size <= setup->m_expect_size);
            void * buf_data = NULL;
            assert_return_code(
                net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_write, setup->m_expect_size, &buf_data),
                0);
            assert_memory_equal(buf_data, setup->m_expect_buf, setup->m_expect_size);

            net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, setup->m_expect_size);
            goto PROCESSED;
        }
    }

    switch (endpoint->m_write_policy.m_type) {
    case test_net_endpoint_write_mock:
        assert(0);
        break;
    case test_net_endpoint_write_noop:
        break;
    case test_net_endpoint_write_keep:
        assert_return_code(
            net_endpoint_buf_append_from_self(
                base_endpoint, endpoint->m_write_policy.m_keep.m_buf_type,
                net_ep_buf_write, 0),
            0);
        break;
    case test_net_endpoint_write_remove:
        net_endpoint_buf_consume(
            base_endpoint, net_ep_buf_write,
            net_endpoint_buf_size(base_endpoint, net_ep_buf_write));
        break;
    case test_net_endpoint_write_link:
        if (endpoint->m_write_policy.m_link.m_error_source == net_endpoint_error_source_none) {
            assert_true(endpoint->m_link);
            test_net_endpoint_t other = test_net_endpoint_link_other(endpoint->m_link, endpoint);
            net_endpoint_t base_other_endpoint = net_endpoint_from_data(other);
            test_net_endpoint_write_delay(
                driver, base_endpoint, base_other_endpoint,
                endpoint->m_write_policy.m_link.m_write_delay_ms);
        }
        else {
            test_net_endpoint_error(
                driver, base_endpoint,
                endpoint->m_write_policy.m_link.m_error_source,
                endpoint->m_write_policy.m_link.m_error_no,
                endpoint->m_write_policy.m_link.m_error_msg,
                endpoint->m_write_policy.m_link.m_write_delay_ms);
        }
        break;
    case test_net_endpoint_write_error:
        test_net_endpoint_error(
            driver, base_endpoint,
            endpoint->m_write_policy.m_error.m_error_source,
            endpoint->m_write_policy.m_error.m_error_no,
            endpoint->m_write_policy.m_error.m_error_msg,
            endpoint->m_write_policy.m_error.m_error_delay_ms);
        break;
    }

PROCESSED:
    if (net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) {
        net_endpoint_set_is_writing(base_endpoint, 0);
        return;
    } else {
        goto PROCESS_AGAIN;
    }
}

/*keep*/
static void test_net_driver_do_expect_write_keep(
    test_net_driver_t driver, uint32_t id, net_endpoint_buf_type_t buf_type)
{
    struct test_net_endpoint_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_write_setup));
    setup->m_write_policy.m_type = test_net_endpoint_write_keep;
    setup->m_write_policy.m_keep.m_buf_type = buf_type;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    
    expect_value(test_net_endpoint_write, id, id);
    will_return(test_net_endpoint_write, setup);
}

void test_net_next_endpoint_expect_write_keep(
    test_net_driver_t driver, net_endpoint_buf_type_t buf_type)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_driver_do_expect_write_keep(driver, net_schedule_next_endpoint_id(schedule), buf_type);
}

void test_net_endpoint_expect_write_keep(
    net_endpoint_t base_endpoint, net_endpoint_buf_type_t buf_type)
{
    test_net_driver_t driver = test_net_driver_cast(net_endpoint_driver(base_endpoint));
    assert_true(driver);
    test_net_driver_do_expect_write_keep(driver, net_endpoint_id(base_endpoint), buf_type);
}

/*noop*/
static void test_net_driver_do_expect_write_noop(test_net_driver_t driver, uint32_t id) {
    struct test_net_endpoint_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_write_setup));
    setup->m_write_policy.m_type = test_net_endpoint_write_noop;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    
    expect_value(test_net_endpoint_write, id, id);
    will_return(test_net_endpoint_write, setup);
}

void test_net_next_endpoint_expect_write_noop(test_net_driver_t driver) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_driver_do_expect_write_noop(driver, net_schedule_next_endpoint_id(schedule));
}

void test_net_endpoint_expect_write_noop(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_driver_do_expect_write_noop(driver, net_endpoint_id(base_endpoint));
}

/*error*/
static void test_net_driver_do_expect_write_error(
    test_net_driver_t driver, uint32_t id,
    net_endpoint_error_source_t error_source, int error_no, const char * error_msg, int64_t delay_ms)
{
    struct test_net_endpoint_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_write_setup));
    setup->m_write_policy.m_type = test_net_endpoint_write_error;
    setup->m_write_policy.m_error.m_error_source = error_source;
    setup->m_write_policy.m_error.m_error_no = error_no;
    setup->m_write_policy.m_error.m_error_msg = error_msg ? mem_buffer_strdup(&driver->m_setup_buffer, error_msg) : NULL;
    setup->m_write_policy.m_error.m_error_delay_ms = delay_ms;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    
    expect_value(test_net_endpoint_write, id, id);
    will_return(test_net_endpoint_write, setup);
}

void test_net_next_endpoint_expect_write_error(
    test_net_driver_t driver,
    net_endpoint_error_source_t error_source, int err_no, const char * error_msg, int64_t delay_ms)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_driver_do_expect_write_error(
        driver, net_schedule_next_endpoint_id(schedule), error_source, err_no, error_msg, delay_ms);
}

void test_net_endpoint_expect_write_error(
    net_endpoint_t base_endpoint,
    net_endpoint_error_source_t error_source, int err_no, const char * error_msg, int64_t delay_ms)
{
    test_net_driver_t driver = test_net_driver_cast(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = test_net_endpoint_cast(base_endpoint);

    if (endpoint->m_write_policy.m_type == test_net_endpoint_write_link) {
        endpoint->m_write_policy.m_link.m_write_delay_ms = delay_ms;
        endpoint->m_write_policy.m_link.m_error_source = error_source;
        endpoint->m_write_policy.m_link.m_error_no = err_no;
        endpoint->m_write_policy.m_link.m_error_msg =
            error_msg ? mem_buffer_strdup(&driver->m_setup_buffer, error_msg) : NULL;
    }
    else {
        test_net_endpoint_write_policy_clear(endpoint);
        test_net_driver_do_expect_write_error(
            driver, net_endpoint_id(base_endpoint), error_source, err_no, error_msg, delay_ms);
    }
}
