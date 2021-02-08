#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_kcp_tests.h"
#include "net_kcp_endpoint.h"
#include "net_kcp_pair_testenv.h"

static int setup(void **state) {
    net_kcp_pair_testenv_t env = net_kcp_pair_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_kcp_pair_testenv_t env = *state;
    net_kcp_pair_testenv_free(env);
    return 0;
}

static void net_kcp_pair_basic(void **state) {
    /* net_kcp_pair_testenv_t env = *state; */
    /* net_kcp_endpoint_t cli_ep = net_kcp_testenv_stream_ep_create(env->m_env); */
    /* net_endpoint_t cli_ep_base = net_kcp_endpoint_base_endpoint(cli_ep); */
    /* net_endpoint_set_remote_address(cli_ep_base, net_acceptor_address(env->m_acceptor)); */
    /* net_kcp_endpoint_set_path(cli_ep, "/a/b"); */
    
    /* net_endpoint_t cli_underline = net_kcp_endpoint_underline(cli_ep_base); */
    /* assert_true(cli_underline != NULL); */

    /* test_net_endpoint_expect_connect_to_acceptor(cli_underline, "1.2.3.4:5678", 0, 0); */
    
    /* assert_true(net_endpoint_connect(cli_ep_base) == 0); */

    /* test_net_driver_run(env->m_env->m_tdriver, 0); */
    /* assert_string_equal( */
    /*     net_endpoint_state_str(net_endpoint_state(cli_ep_base)), */
    /*     net_endpoint_state_str(net_endpoint_state_established)); */

    /* net_endpoint_t svr_ep = net_kcp_pair_testenv_get_svr_stream(env, cli_ep_base); */
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

int net_kcp_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_kcp_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
