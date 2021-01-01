#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

struct test_net_endpoint_write_setup {
    void * m_expect_buf;
    void * m_expect_size;
};

void test_net_endpoint_write(net_endpoint_t base_endpoint) {
    test_net_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    uint32_t id = net_endpoint_id(base_endpoint);
    check_expected(id);

    struct test_net_endpoint_write_setup * setup = mock_type(struct test_net_endpoint_write_setup *);
    assert_ptr_not_equal(setup, NULL);

    uint32_t size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
}
