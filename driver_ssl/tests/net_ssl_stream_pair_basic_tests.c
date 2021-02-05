#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_stream_driver.h"
#include "net_ssl_stream_endpoint.h"
#include "net_ssl_stream_pair_testenv.h"

static int setup(void **state) {
    net_ssl_stream_pair_testenv_t env = net_ssl_stream_pair_testenv_create();
    *state = env;
    assert_true(
        net_ssl_stream_driver_svr_prepaired(env->m_env->m_stream_driver) == 0);
    return 0;
}

static int teardown(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_pair_testenv_free(env);
    return 0;
}

static void net_ssl_stream_pair_basic(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env->m_env, NULL);
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    net_endpoint_set_remote_address(cli_stream_base, net_acceptor_address(env->m_acceptor));

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_to_acceptor(cli_ssl_base, "1.2.3.4:5678", 0, 0);
    
    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    test_net_driver_run(env->m_env->m_tdriver, 0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_ssl_stream_endpoint_t svr_stream = net_ssl_stream_pair_testenv_get_svr_ep(env, cli_stream);
    assert_true(svr_stream != NULL);

    net_endpoint_t svr_stream_base = net_ssl_stream_endpoint_base_endpoint(svr_stream);
        
    /*client -> server*/
    assert_true(net_endpoint_buf_append(cli_stream_base, net_ep_buf_write, "abcd", 4) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(svr_stream_base, net_ep_buf_read, "abcd", 4);

    /*server -> client*/
    assert_true(net_endpoint_buf_append(svr_stream_base, net_ep_buf_write, "efgh", 4) == 0);
    test_net_driver_run(env->m_env->m_tdriver, 0);

    test_net_endpoint_assert_buf_memory(cli_stream_base, net_ep_buf_read, "efgh", 4);
}

static void net_ssl_stream_pair_delay(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env->m_env, NULL);
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    net_endpoint_set_remote_address(cli_stream_base, net_acceptor_address(env->m_acceptor));

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_to_acceptor(cli_ssl_base, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_established));
}

static void net_ssl_stream_pair_input_handshake(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env->m_env, NULL);
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    net_endpoint_set_remote_address(cli_stream_base, net_acceptor_address(env->m_acceptor));

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_to_acceptor(cli_ssl_base, "1.2.3.4:5678", 0, 0);
    
    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_true(
        net_endpoint_buf_append(cli_stream_base, net_ep_buf_write, "abcd", 4) == 0);
    
    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_ssl_stream_endpoint_t svr_stream = net_ssl_stream_pair_testenv_get_svr_ep(env, cli_stream);
    assert_true(svr_stream != NULL);

    net_endpoint_t svr_stream_base = net_ssl_stream_endpoint_base_endpoint(svr_stream);
    
    test_net_endpoint_assert_buf_memory(svr_stream_base, net_ep_buf_read, "abcd", 4);
}

static void net_ssl_stream_pair_input_connecting(void **state) {
    net_ssl_stream_pair_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env->m_env, NULL);
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    net_endpoint_set_remote_address(cli_stream_base, net_acceptor_address(env->m_acceptor));

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);

    test_net_endpoint_expect_connect_to_acceptor(cli_ssl_base, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_true(
        net_endpoint_buf_append(cli_stream_base, net_ep_buf_write, "abcd", 4) == 0);
    
    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_ssl_stream_endpoint_t svr_stream = net_ssl_stream_pair_testenv_get_svr_ep(env, cli_stream);
    assert_true(svr_stream != NULL);

    net_endpoint_t svr_stream_base = net_ssl_stream_endpoint_base_endpoint(svr_stream);
    
    test_net_endpoint_assert_buf_memory(svr_stream_base, net_ep_buf_read, "abcd", 4);
}

int net_ssl_stream_pair_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_stream_pair_basic, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_pair_delay, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_pair_input_handshake, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_pair_input_connecting, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
