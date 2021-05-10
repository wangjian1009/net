#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "test_net_tl_op.h"
#include "test_ws_endpoint_receiver.h"

struct test_net_ws_endpoint_setup {
    test_net_driver_t m_tdriver;
    struct test_net_ws_endpoint_action m_action;
    int64_t m_delay_ms;
};

void test_net_ws_endpoint_apply_setup(net_ws_endpoint_t endpoint, struct test_net_ws_endpoint_setup * setup) {
    test_net_ws_endpoint_apply_action(setup->m_tdriver, endpoint, &setup->m_action, setup->m_delay_ms);
}

/*回调函数 */
void test_net_ws_endpoint_on_msg_text(void * ctx, net_ws_endpoint_t endpoint, const char * msg) {
    uint32_t id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    check_expected(id);
    check_expected(msg);

    struct test_net_ws_endpoint_setup * setup = mock_type(struct test_net_ws_endpoint_setup *);
    test_net_ws_endpoint_apply_setup(endpoint, setup);
}

void test_net_ws_endpoint_on_msg_bin(void * ctx, net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    uint32_t id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    check_expected(id);
    check_expected(msg);

    struct test_net_ws_endpoint_setup * setup = mock_type(struct test_net_ws_endpoint_setup *);
    test_net_ws_endpoint_apply_setup(endpoint, setup);
}

void test_net_ws_endpoint_on_close(
    void * ctx, net_ws_endpoint_t endpoint, uint16_t status_code, const void * msg, uint32_t msg_len)
{
    uint32_t id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    check_expected(id);
    check_expected(status_code);    
    check_expected(msg);

    struct test_net_ws_endpoint_setup * setup = mock_type(struct test_net_ws_endpoint_setup *);
    test_net_ws_endpoint_apply_setup(endpoint, setup);
}

void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint) {
    net_ws_endpoint_set_callback(
        endpoint,
        NULL,
        NULL,
        test_net_ws_endpoint_on_msg_text,
        test_net_ws_endpoint_on_msg_bin,
        test_net_ws_endpoint_on_close,
        NULL);
}

/*设置函数 */
struct test_net_ws_endpoint_setup *
test_net_ws_endpoint_expect_create_setup(
    test_net_driver_t tdriver, test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    struct test_net_ws_endpoint_setup * setup =
        mem_buffer_alloc(&tdriver->m_setup_buffer, sizeof(struct test_net_ws_endpoint_setup));

    setup->m_tdriver = tdriver;
    setup->m_action = *action;
    setup->m_delay_ms = delay_ms;
    
    switch(setup->m_action.m_type) {
    case test_net_ws_endpoint_op_noop:
    case test_net_ws_endpoint_op_disable:
    case test_net_ws_endpoint_op_error:
    case test_net_ws_endpoint_op_delete:
        break;
    case test_net_ws_endpoint_op_close:
        setup->m_action.m_close.m_msg =
            setup->m_action.m_close.m_msg
            ? mem_buffer_strdup(&tdriver->m_setup_buffer, setup->m_action.m_close.m_msg)
            : NULL;
        break;
    }

    return setup;
}

void test_net_ws_endpoint_expect_text_msg(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, const char * i_msg,
    test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    expect_value(test_net_ws_endpoint_on_msg_text, id, net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint)));
    if (i_msg) {
        char * msg = mem_buffer_strdup(&tdriver->m_setup_buffer, i_msg);
        expect_string(test_net_ws_endpoint_on_msg_text, msg, msg);
    }
    else {
        expect_any(test_net_ws_endpoint_on_msg_text, msg);
    }

    struct test_net_ws_endpoint_setup * setup = test_net_ws_endpoint_expect_create_setup(tdriver, action, delay_ms);
    will_return(test_net_ws_endpoint_on_msg_text, setup);
}

void test_net_ws_endpoint_expect_bin_msg(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, const void * i_msg, uint32_t msg_len,
    test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    expect_value(test_net_ws_endpoint_on_msg_bin, id, net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint)));

    if (i_msg) {
        void * msg = mem_buffer_alloc(&tdriver->m_setup_buffer, msg_len);
        memcpy(msg, i_msg, msg_len);
        expect_memory(test_net_ws_endpoint_on_msg_bin, msg, msg, msg_len);
    }
    else {
        expect_any(test_net_ws_endpoint_on_msg_bin, msg);
    }

    struct test_net_ws_endpoint_setup * setup = test_net_ws_endpoint_expect_create_setup(tdriver, action, delay_ms);
    will_return(test_net_ws_endpoint_on_msg_bin, setup);
}

void test_net_ws_endpoint_expect_close(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, uint16_t status_code, const void * i_msg, uint32_t msg_len,
    test_net_ws_endpoint_action_t action, int64_t delay_ms)
{
    expect_value(test_net_ws_endpoint_on_close, id, net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint)));

    expect_value(test_net_ws_endpoint_on_close, status_code, status_code);
        
    if (i_msg) {
        void * msg = mem_buffer_alloc(&tdriver->m_setup_buffer, msg_len);
        memcpy(msg, i_msg, msg_len);
        expect_memory(test_net_ws_endpoint_on_close, msg, msg, msg_len);
    }
    else {
        expect_any(test_net_ws_endpoint_on_close, msg);
    }

    struct test_net_ws_endpoint_setup * setup = test_net_ws_endpoint_expect_create_setup(tdriver, action, delay_ms);
    will_return(test_net_ws_endpoint_on_close, setup);
}
