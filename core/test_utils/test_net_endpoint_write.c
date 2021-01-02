#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

struct test_net_endpoint_write_setup {
    int64_t m_duration_ms;
    void * m_expect_buf;
    uint32_t m_expect_size;
};

void test_net_endpoint_write(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);

    struct test_net_endpoint_write_setup * setup = mock_type(struct test_net_endpoint_write_setup *);
    assert_ptr_not_equal(setup, NULL);

    uint32_t size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
}

static void test_net_driver_do_expect_write_noop(test_net_driver_t driver, uint32_t id, int64_t duration_ms) {
    struct test_net_endpoint_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_write_setup));
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    setup->m_duration_ms = duration_ms;
    
    expect_value_count(test_net_endpoint_write, id, id, INT_MAX);
    will_return_count(test_net_endpoint_write, setup, INT_MAX);
}

void test_net_driver_expect_next_endpoint_write_noop(test_net_driver_t driver, int64_t duration_ms) {
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_driver_do_expect_write_noop(driver, net_schedule_next_endpoint_id(schedule), duration_ms);
}

void test_net_endpoint_expect_write_noop(net_endpoint_t base_endpoint, int64_t duration_ms) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_driver_do_expect_write_noop(driver, net_endpoint_id(base_endpoint), duration_ms);
}
