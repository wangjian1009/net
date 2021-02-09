#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_http2_tests.h"
#include "net_http2_stream_endpoint.h"
#include "net_http2_stream_pair_testenv.h"

static int setup(void **state) {
    net_http2_stream_pair_testenv_t env = net_http2_stream_pair_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_http2_stream_pair_testenv_t env = *state;
    net_http2_stream_pair_testenv_free(env);
    return 0;
}

static void net_http2_stream_pair_basic(void **state) {
    net_http2_stream_pair_testenv_t env = *state;
    net_http2_stream_endpoint_t cli_ep = net_http2_testenv_stream_ep_create(env->m_env);
    net_endpoint_t cli_ep_base = net_http2_stream_endpoint_base_endpoint(cli_ep);
    net_endpoint_set_remote_address(cli_ep_base, net_acceptor_address(env->m_acceptor));
    
    net_endpoint_t cli_control = net_http2_stream_endpoint_control(cli_ep_base);
    assert_true(cli_control != NULL);

    test_net_endpoint_expect_connect_to_acceptor(cli_control, "1.2.3.4:5678", 0, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    test_net_driver_run(env->m_env->m_tdriver, 0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_endpoint_t svr_ep = net_http2_stream_pair_testenv_get_svr_stream(env, cli_ep_base);
    assert_true(svr_ep != NULL);

    /*client -> server*/
    assert_true(net_endpoint_buf_append(cli_ep_base, net_ep_buf_write, "abcd", 4) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(svr_ep, net_ep_buf_read, "abcd", 4);

    /*server -> client*/
    assert_true(net_endpoint_buf_append(svr_ep, net_ep_buf_write, "efgh", 4) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(cli_ep_base, net_ep_buf_read, "efgh", 4);
}

static void net_http2_stream_pair_delay(void **state) {
    net_http2_stream_pair_testenv_t env = *state;
    net_http2_stream_endpoint_t cli_ep = net_http2_testenv_stream_ep_create(env->m_env);
    net_endpoint_t cli_ep_base = net_http2_stream_endpoint_base_endpoint(cli_ep);
    net_endpoint_set_remote_address(cli_ep_base, net_acceptor_address(env->m_acceptor));

    net_endpoint_t cli_control = net_http2_stream_endpoint_control(cli_ep_base);
    assert_true(cli_control != NULL);

    test_net_endpoint_expect_connect_to_acceptor(cli_control, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));
}

static void net_http2_stream_pair_input_connecting(void **state) {
    net_http2_stream_pair_testenv_t env = *state;
    net_http2_stream_endpoint_t cli_ep = net_http2_testenv_stream_ep_create(env->m_env);
    net_endpoint_t cli_ep_base = net_http2_stream_endpoint_base_endpoint(cli_ep);
    net_endpoint_set_remote_address(cli_ep_base, net_acceptor_address(env->m_acceptor));

    net_endpoint_t cli_control = net_http2_stream_endpoint_control(cli_ep_base);
    assert_true(cli_control != NULL);

    test_net_endpoint_expect_connect_to_acceptor(cli_control, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_true(
        net_endpoint_buf_append(cli_ep_base, net_ep_buf_write, "abcd", 4) == 0);
    
    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_endpoint_t svr_ep = net_http2_stream_pair_testenv_get_svr_stream(env, cli_ep_base);
    assert_true(svr_ep != NULL);
    
    test_net_endpoint_assert_buf_memory(svr_ep, net_ep_buf_read, "abcd", 4);
}

int net_http2_stream_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_stream_pair_basic, setup, teardown),
		cmocka_unit_test_setup_teardown(net_http2_stream_pair_delay, setup, teardown),
		cmocka_unit_test_setup_teardown(net_http2_stream_pair_input_connecting, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
