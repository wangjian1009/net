#include "cmocka_all.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"

struct test_net_endpoint_connect_setup {
    net_endpoint_state_t m_state;
    net_endpoint_error_source_t m_error_source;
    uint32_t m_error_no;
    const char * m_error_msg;
};

void test_net_endpoint_connect_will_return(
    test_net_driver_t driver,
    net_endpoint_state_t state,
    net_endpoint_error_source_t error_source, uint32_t error_no, const char * error_msg)
{
    struct test_net_endpoint_connect_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_endpoint_connect_setup));

    setup->m_state = state;
    setup->m_error_source = error_source;
    setup->m_error_no = error_no;
    setup->m_error_msg = error_msg;

    will_return(test_net_endpoint_connect, setup);
}

int test_net_endpoint_connect_apply_setup(
    net_endpoint_t base_endpoint,
    struct test_net_endpoint_connect_setup * setup)
{
    net_endpoint_set_error(base_endpoint, setup->m_error_source, setup->m_error_no, setup->m_error_msg);
    return net_endpoint_set_state(base_endpoint, setup->m_state);
}

int test_net_endpoint_connect(net_endpoint_t base_endpoint) {
    char remote_addr[256];
    net_address_to_string(remote_addr, sizeof(remote_addr), net_endpoint_remote_address(base_endpoint));
    
    check_expected(remote_addr);

    struct test_net_endpoint_connect_setup * setup = mock_type(struct test_net_endpoint_connect_setup *);
    if (setup == NULL) return -1;

    return test_net_endpoint_connect_apply_setup(base_endpoint, setup);
}

void test_net_driver_expect_connect_success(test_net_driver_t driver, const char * target, int64_t delay_ms) {
    expect_string(test_net_endpoint_connect, remote_addr, target);

    if (delay_ms == 0) {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_established, net_endpoint_error_source_user, 0, NULL);
    }
    else {
        test_net_endpoint_connect_will_return(
            driver, net_endpoint_state_connecting, net_endpoint_error_source_user, 0, NULL);

    }
}
