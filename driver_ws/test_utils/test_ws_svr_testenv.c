#include "cmocka_all.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_acceptor.h"
#include "test_net_endpoint.h"
#include "test_net_tl_op.h"
#include "net_ws_endpoint.h"
#include "net_ws_protocol.h"
#include "test_ws_svr_testenv.h"
#include "test_ws_svr_mock_svr.h"

test_ws_svr_testenv_t
test_ws_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em) {
    test_ws_svr_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct test_ws_svr_testenv));
    env->m_em = em;
    env->m_schedule = schedule;
    env->m_driver = driver;

    TAILQ_INIT(&env->m_svrs);
    
    return env;
}

void test_ws_svr_testenv_free(test_ws_svr_testenv_t env) {
    while(!TAILQ_EMPTY(&env->m_svrs)) {
        test_ws_svr_mock_svr_free(TAILQ_FIRST(&env->m_svrs));
    }

    mem_free(test_allocrator(), env);
}

net_ws_endpoint_t
test_ws_svr_testenv_svr_endpoint(test_ws_svr_testenv_t env, net_ws_endpoint_t cli_endpoint) {
    net_endpoint_t other_base = test_net_endpoint_linked_other(env->m_driver, net_ws_endpoint_base_endpoint(cli_endpoint));
    return net_ws_endpoint_cast(other_base);
}

enum test_ws_svr_testenv_send_msg_op_type {
    test_ws_svr_testenv_send_msg_op_bin_msg,
    test_ws_svr_testenv_send_msg_op_text_msg,
    test_ws_svr_testenv_send_msg_op_close,
};

struct test_ws_svr_testenv_op_data {
    uint32_t m_ep_id;
    enum test_ws_svr_testenv_send_msg_op_type m_op_type;
    union {
        char * m_text_msg;
        struct {
            void * m_data;
            uint32_t m_len;
        } m_bin_msg;
        struct {
            uint16_t m_status_code;
            char * m_msg;
        } m_close;
    };
};

static void test_ws_svr_testenv_op_cb(void * ctx, test_net_tl_op_t op) {
    struct test_ws_svr_testenv_op_data * op_data = ctx;

    assert_true(op_data->m_ep_id != 0);

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(op->m_driver));
    net_endpoint_t base_endpoint = net_endpoint_find(schedule, op_data->m_ep_id);
    if (base_endpoint == NULL) return;

    net_ws_endpoint_t endpoint = net_ws_endpoint_cast(base_endpoint);
    assert_true(endpoint);

    switch(op_data->m_op_type) {
    case test_ws_svr_testenv_send_msg_op_bin_msg:
        net_ws_endpoint_send_msg_bin(endpoint, op_data->m_bin_msg.m_data, op_data->m_bin_msg.m_len);
        break;
    case test_ws_svr_testenv_send_msg_op_text_msg:
        net_ws_endpoint_send_msg_text(endpoint, op_data->m_text_msg);
        break;
    case test_ws_svr_testenv_send_msg_op_close:
        net_ws_endpoint_close(endpoint, op_data->m_close.m_status_code, op_data->m_close.m_msg);
        break;
    }
}

void test_ws_svr_testenv_send_bin_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, void const * msg, uint32_t msg_len, int64_t delay_ms)
{
    test_net_tl_op_t op =
        test_net_tl_op_create(
            env->m_driver, delay_ms,
            sizeof(struct test_ws_svr_testenv_op_data),
            test_ws_svr_testenv_op_cb,
            NULL, NULL);
    
    struct test_ws_svr_testenv_op_data * op_data = test_net_tl_op_data(op);

    op_data->m_ep_id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    op_data->m_op_type = test_ws_svr_testenv_send_msg_op_bin_msg;
    op_data->m_bin_msg.m_data = mem_buffer_alloc(&env->m_driver->m_setup_buffer, msg_len);
    memcmp(op_data->m_bin_msg.m_data, msg, msg_len);
    op_data->m_bin_msg.m_len = msg_len;
}

void test_ws_svr_testenv_send_text_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, const char * msg, int64_t delay_ms)
{
    test_net_tl_op_t op =
        test_net_tl_op_create(
            env->m_driver, delay_ms,
            sizeof(struct test_ws_svr_testenv_op_data),
            test_ws_svr_testenv_op_cb,
            NULL, NULL);
    
    struct test_ws_svr_testenv_op_data * op_data = test_net_tl_op_data(op);

    op_data->m_ep_id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    op_data->m_op_type = test_ws_svr_testenv_send_msg_op_text_msg;
    op_data->m_text_msg = mem_buffer_strdup(&env->m_driver->m_setup_buffer, msg);
}

void test_ws_svr_testenv_close(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, uint16_t status_code, const char * msg, int64_t delay_ms)
{
    test_net_tl_op_t op =
        test_net_tl_op_create(
            env->m_driver, delay_ms,
            sizeof(struct test_ws_svr_testenv_op_data),
            test_ws_svr_testenv_op_cb,
            NULL, NULL);
    
    struct test_ws_svr_testenv_op_data * op_data = test_net_tl_op_data(op);

    op_data->m_ep_id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    op_data->m_op_type = test_ws_svr_testenv_send_msg_op_close;
    op_data->m_close.m_status_code = status_code;
    op_data->m_close.m_msg = mem_buffer_strdup(&env->m_driver->m_setup_buffer, msg);
}
