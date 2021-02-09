#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_http2_tests.h"
#include "net_http2_testenv.h"

static int setup(void **state) {
    net_http2_testenv_t env = net_http2_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_http2_testenv_t env = *state;
    net_http2_testenv_free(env);
    return 0;
}

static void net_http2_pair_basic(void **state) {
    net_http2_testenv_t env = *state;

    net_http2_endpoint_t cli_ep;
    net_http2_endpoint_t svr_ep;
    net_http2_testenv_create_pair_established(env, &cli_ep, &svr_ep, "1.1.2.3:5678");

    test_net_driver_run(env->m_tdriver, 0);

    /* /\*验证svr状态 *\/ */
    /* assert_string_equal( */
    /*     net_http2_endpoint_state_str(net_http2_endpoint_state(svr_ep)), */
    /*     net_http2_endpoint_state_str(net_http2_endpoint_state_streaming)); */

    /* /\*验证cli状态 *\/ */
    /* assert_string_equal( */
    /*     net_http2_endpoint_state_str(net_http2_endpoint_state(cli_ep)), */
    /*     net_http2_endpoint_state_str(net_http2_endpoint_state_streaming)); */

    /*client -> server*/
    /* test_net_http2_endpoint_expect_text_msg(svr_ep, "abcd"); */
    /* assert_true(net_http2_endpoint_send_msg_text(cli_ep, "abcd") == 0); */
    /* test_net_driver_run(env->m_tdriver, 0); */

    /* test_net_http2_endpoint_expect_bin_msg(svr_ep, "efgh", 4); */
    /* assert_true(net_http2_endpoint_send_msg_bin(cli_ep, "efgh", 4) == 0); */
    /* test_net_driver_run(env->m_tdriver, 0); */

    /* /\*server -> client*\/ */
    /* test_net_http2_endpoint_expect_text_msg(cli_ep, "hijk"); */
    /* assert_true(net_http2_endpoint_send_msg_text(svr_ep, "hijk") == 0); */
    /* test_net_driver_run(env->m_tdriver, 0); */

    /* test_net_http2_endpoint_expect_bin_msg(cli_ep, "lmno", 4); */
    /* assert_true(net_http2_endpoint_send_msg_bin(svr_ep, "lmno", 4) == 0); */
    /* test_net_driver_run(env->m_tdriver, 0); */
}

int net_http2_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
