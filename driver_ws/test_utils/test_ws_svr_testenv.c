#include "cmocka_all.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "test_net_endpoint.h"
#include "net_ssl_stream_endpoint.h"
#include "net_ssl_endpoint.h"
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
    net_endpoint_t cli_base_endpoint = net_ws_endpoint_base_endpoint(cli_endpoint);

    net_ssl_stream_endpoint_t cli_ssl_stream_endpoit = net_ssl_stream_endpoint_cast(cli_base_endpoint);
    if (cli_ssl_stream_endpoit == NULL) {
        net_endpoint_t other_base = test_net_endpoint_linked_other(env->m_driver, cli_base_endpoint);
        return other_base ? net_ws_endpoint_cast(other_base) : NULL;
    }

    net_ssl_endpoint_t cli_ssl_endpoint = net_ssl_stream_endpoint_underline(cli_ssl_stream_endpoit);
    assert_true(cli_ssl_endpoint != NULL);

    net_endpoint_t svr_ssl_base_endpoint =
        test_net_endpoint_linked_other(env->m_driver, net_ssl_endpoint_base_endpoint(cli_ssl_endpoint));
    if (svr_ssl_base_endpoint == NULL) return NULL;

    net_ssl_endpoint_t svr_ssl_endpoint = net_ssl_endpoint_cast(svr_ssl_base_endpoint);
    assert_true(svr_ssl_endpoint);

    net_ssl_stream_endpoint_t svr_ssl_stream_endpoit = net_ssl_endpoint_stream(svr_ssl_endpoint);
    if (svr_ssl_stream_endpoit == NULL) return NULL;

    return net_ws_endpoint_cast(net_ssl_stream_endpoint_base_endpoint(svr_ssl_stream_endpoit));
}

void test_ws_svr_testenv_send_bin_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, void const * msg, uint32_t msg_len, int64_t delay_ms)
{
    struct test_net_ws_endpoint_action action;
    action.m_type = test_net_ws_endpoint_op_bin_msg;
    action.m_bin_msg.m_msg_size = msg_len;
    action.m_bin_msg.m_msg = NULL;

    if (msg_len) {
        action.m_bin_msg.m_msg = mem_buffer_alloc(&env->m_driver->m_setup_buffer, msg_len);
        memcpy((void*)action.m_bin_msg.m_msg, msg, msg_len);
    }
    
    test_net_ws_endpoint_apply_action(env->m_driver, endpoint, &action, delay_ms);
}

void test_ws_svr_testenv_send_text_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, const char * msg, int64_t delay_ms)
{
    struct test_net_ws_endpoint_action action;
    action.m_type = test_net_ws_endpoint_op_text_msg;
    action.m_text_msg.m_msg = mem_buffer_strdup(&env->m_driver->m_setup_buffer, msg);
    test_net_ws_endpoint_apply_action(env->m_driver, endpoint, &action, delay_ms);
}

void test_ws_svr_testenv_close(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, uint16_t status_code, const char * msg, int64_t delay_ms)
{
    struct test_net_ws_endpoint_action action;
    action.m_type = test_net_ws_endpoint_op_close;
    action.m_close.m_status_code = status_code;
    action.m_close.m_msg = msg ? mem_buffer_strdup(&env->m_driver->m_setup_buffer, msg) : NULL;

    test_net_ws_endpoint_apply_action(env->m_driver, endpoint, &action, delay_ms);
}
