#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_pair_testenv.h"

static int setup(void **state) {
    net_ssl_pair_testenv_t env = net_ssl_pair_testenv_create();
    *state = env;
    assert_true(
        net_ssl_svr_driver_prepaired(env->m_env->m_svr_driver) == 0);
    return 0;
}

static int teardown(void **state) {
    net_ssl_pair_testenv_t env = *state;
    net_ssl_pair_testenv_free(env);
    return 0;
}

static void net_ssl_pair_basic(void **state) {
    net_ssl_pair_testenv_t env = *state;
    net_endpoint_t cli_ep = net_ssl_testenv_cli_ep_create(env->m_env);
    net_endpoint_set_remote_address(cli_ep, net_acceptor_address(env->m_acceptor));

    net_endpoint_t cli_underline = net_ssl_testenv_cli_ep_undline(cli_ep);
    assert_true(cli_underline != NULL);

    test_net_endpoint_expect_connect_to_acceptor(cli_underline, "1.2.3.4:5678", 0, 0);
    
    assert_true(net_endpoint_connect(cli_ep) == 0);

    test_net_driver_run(env->m_env->m_tdriver, 0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep)),
        net_endpoint_state_str(net_endpoint_state_established));
        
    //assert_true(net_endpoint_buf_append(cli_ep, net_ep_buf_write, "abcd", 4) == -1);
}

static void net_ssl_pair_delay(void **state) {
    net_ssl_pair_testenv_t env = *state;
    net_endpoint_t cli_ep = net_ssl_testenv_cli_ep_create(env->m_env);
    net_endpoint_set_remote_address(cli_ep, net_acceptor_address(env->m_acceptor));

    net_endpoint_t cli_underline = net_ssl_testenv_cli_ep_undline(cli_ep);
    assert_true(cli_underline != NULL);

    test_net_endpoint_expect_connect_to_acceptor(cli_underline, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_env->m_tdriver, 500);
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep)),
        net_endpoint_state_str(net_endpoint_state_established));
        
    test_net_driver_run(env->m_env->m_tdriver, 0);
    //assert_true(net_endpoint_buf_append(cli_ep, net_ep_buf_write, "abcd", 4) == -1);
}

int net_ssl_pair_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_pair_basic, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_pair_delay, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
