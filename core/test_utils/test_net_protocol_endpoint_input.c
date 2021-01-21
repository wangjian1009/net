#include <assert.h>
#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "test_net_protocol_endpoint_link.h"
#include "test_net_tl_op.h"

struct test_net_protocol_endpoint_input_setup {
    struct test_net_protocol_endpoint_input_policy m_input_policy;
    void * m_expect_buf;
    uint32_t m_expect_size;
};

struct test_net_protocol_endpoint_input_op {
    uint32_t m_ep_id;
    uint32_t m_data_size;
};

int test_net_protocol_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}

void test_net_protocol_endpoint_input_policy_clear(test_net_protocol_endpoint_t endpoint) {
    switch(endpoint->m_input_policy.m_type) {
    case test_net_protocol_endpoint_input_mock:
    case test_net_protocol_endpoint_input_keep:
    case test_net_protocol_endpoint_input_remove:
        break;
    case test_net_protocol_endpoint_input_link:
        test_net_protocol_endpoint_link_free(endpoint->m_input_policy.m_link.m_link);
        assert(endpoint->m_input_policy.m_link.m_link == NULL);
        break;
    }

    endpoint->m_input_policy.m_type = test_net_protocol_endpoint_input_mock;
}

static void test_net_driver_do_expect_input_keep(
    test_net_driver_t driver, uint32_t id, net_endpoint_buf_type_t buf_type, int64_t duration_ms)
{
    struct test_net_protocol_endpoint_input_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_protocol_endpoint_input_setup));
    setup->m_input_policy.m_type = test_net_protocol_endpoint_input_keep;
    setup->m_input_policy.m_keep.m_buf_type = buf_type;
    setup->m_expect_buf = NULL;
    setup->m_expect_size = 0;
    
    expect_value(test_net_protocol_endpoint_input, id, id);
    will_return(test_net_protocol_endpoint_input, setup);
}

void test_net_protocol_endpoint_expect_input_keep(
    net_endpoint_t base_endpoint, net_endpoint_buf_type_t buf_type)
{
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    test_net_driver_do_expect_input_keep(driver, net_endpoint_id(base_endpoint), buf_type, 0);
}
