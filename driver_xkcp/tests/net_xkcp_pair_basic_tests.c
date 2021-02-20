#include "cmocka_all.h"
#include "net_xkcp_tests.h"
#include "net_xkcp_testenv.h"

static int setup(void **state) {
    net_xkcp_testenv_t env = net_xkcp_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_xkcp_testenv_t env = *state;
    net_xkcp_testenv_free(env);
    return 0;
}

static void net_xkcp_pair_basic(void **state) {
    net_xkcp_testenv_t env = *state;

    net_xkcp_acceptor_t acceptor = net_xkcp_testenv_create_acceptor(env, "1.2.3.4:5678", NULL);
    assert_true(acceptor);

    net_xkcp_connector_t connector = net_xkcp_testenv_create_connector(env, "1.2.3.4:5678", NULL);
    assert_true(connector);

    net_xkcp_endpoint_t cli_ep = net_xkcp_testenv_create_cli_ep(env, "1.2.3.4:5678");
    net_endpoint_t cli_ep_base = net_xkcp_endpoint_base_endpoint(cli_ep);

    //test_net_dgram_expect_write_send(net_xkcp_acceptor_dgram(acceptor), 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    /* test_net_driver_run(env->m_env->m_tdriver, 0); */
    /* assert_string_equal( */
    /*     net_endpoint_state_str(net_endpoint_state(cli_ep_base)), */
    /*     net_endpoint_state_str(net_endpoint_state_established)); */

    /* net_endpoint_t svr_ep = net_xkcp_pair_testenv_get_svr_stream(env, cli_ep_base); */
    /* assert_true(svr_ep != NULL); */

    /* /\*client -> server*\/ */
    /* assert_true(net_endpoint_buf_append(cli_ep_base, net_ep_buf_write, "abcd", 4) == 0); */
    /* test_net_driver_run(env->m_env->m_tdriver, 0); */

    /* test_net_endpoint_assert_buf_memory(svr_ep, net_ep_buf_read, "abcd", 4); */

    /* /\*server -> client*\/ */
    /* assert_true(net_endpoint_buf_append(svr_ep, net_ep_buf_write, "efgh", 4) == 0); */
    /* test_net_driver_run(env->m_env->m_tdriver, 0); */

    /* test_net_endpoint_assert_buf_memory(cli_ep_base, net_ep_buf_read, "efgh", 4); */
}

int net_xkcp_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_xkcp_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
