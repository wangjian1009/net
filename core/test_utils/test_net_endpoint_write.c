#include "cmocka_all.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

struct test_net_endpoint_write_setup {
    struct test_net_endpoint_write_policy m_write_policy;
    int64_t m_duration_ms;
    void * m_expect_buf;
    uint32_t m_expect_size;
};

void test_net_endpoint_write_op_cb(void * ctx, test_net_tl_op_t op) {
    net_endpoint_t base_endpoint = ctx;
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert_true(endpoint->m_writing_op = op);
    endpoint->m_writing_op = NULL;

    if (net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) {
        net_endpoint_set_is_writing(base_endpoint, 0);
    }
    else {
        test_net_endpoint_write(base_endpoint);
    }
}

void test_net_endpoint_write(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    assert_true(endpoint->m_writing_op == NULL);

PROCESS_AGAIN:    
    if (endpoint->m_write_policy.m_type == test_net_endpoint_write_mock) {
        uint32_t id = net_endpoint_id(base_endpoint);
        printf("xxxx write 111: %d\n", id);
        check_expected(id);

        struct test_net_endpoint_write_setup * setup = mock_type(struct test_net_endpoint_write_setup *);
        assert_ptr_not_equal(setup, NULL);

        endpoint->m_write_policy = setup->m_write_policy;
        endpoint->m_write_duration_ms = setup->m_duration_ms;

        if (endpoint->m_write_policy.m_type == test_net_endpoint_write_mock) {
            uint32_t buf_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
            assert_true(buf_size <= setup->m_expect_size);
            void * buf_data = NULL;
            assert_return_code(
                net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_write, setup->m_expect_size, &buf_data),
                0);
            assert_memory_equal(buf_data, setup->m_expect_buf, setup->m_expect_size);

            net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, setup->m_expect_size);
            
            if (endpoint->m_write_duration_ms == 0) {
                if (net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)) {
                    net_endpoint_set_is_writing(base_endpoint, 0);
                    return;
                }
                else {
                    goto PROCESS_AGAIN;
                }
            }
            else {
                
            }

            return;
        }
    }

}

static void test_net_driver_do_expect_write_keep(
    test_net_driver_t driver, uint32_t id, net_endpoint_buf_type_t buf_type, int64_t duration_ms)
{
    struct test_net_endpoint_write_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_write_setup));
    setup->m_write_policy.m_type = test_net_endpoint_write_keep;
    setup->m_write_policy.m_keep.m_buf_type = buf_type;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    setup->m_duration_ms = duration_ms;
    
    expect_value(test_net_endpoint_write, id, id);
    will_return(test_net_endpoint_write, setup);
}

void test_net_driver_expect_next_endpoint_write_keep(
    test_net_driver_t driver, net_endpoint_buf_type_t buf_type, int64_t duration_ms)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    test_net_driver_do_expect_write_keep(driver, net_schedule_next_endpoint_id(schedule), buf_type, duration_ms);
}

void test_net_endpoint_expect_write_keep(
    net_endpoint_t base_endpoint, net_endpoint_buf_type_t buf_type, int64_t duration_ms)
{
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_driver_do_expect_write_keep(driver, net_endpoint_id(base_endpoint), buf_type, duration_ms);
}
