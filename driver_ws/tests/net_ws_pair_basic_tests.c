#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"

static int setup(void **state) {
    net_ws_testenv_t env = net_ws_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_testenv_free(env);
    return 0;
}

static void net_ws_pair_basic(void **state) {
    net_ws_testenv_t env = *state;

    net_endpoint_t cli_base;
    net_endpoint_t svr_base;
    net_ws_testenv_cli_create_pair_established(env, &cli_base, &svr_base, "1.1.2.3:5678", "/a/b");

    net_ws_endpoint_t cli_ep = net_ws_endpoint_cast(cli_base);
    net_ws_endpoint_t svr_ep = net_ws_endpoint_cast(svr_base);

    test_net_driver_run(env->m_tdriver, 0);

    /*验证svr状态 */
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(svr_ep)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_streaming));

    assert_string_equal(
        net_ws_endpoint_path(net_ws_endpoint_cast(svr_base)),
        "/a/b");
    
    /*验证cli状态 */
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(cli_ep)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_streaming));
    
    /*client -> server*/
    assert_true(net_ws_endpoint_send_msg_text(cli_ep, "abcd") == 0);
    test_net_driver_run(env->m_tdriver, 0);

    /* test_net_endpoint_assert_buf_memory(svr_ep, net_ep_buf_read, "abcd", 4); */

    /*server -> client*/
    /* assert_true(net_endpoint_buf_append(svr_ep, net_ep_buf_write, "efgh", 4) == 0); */
    /* test_net_driver_run(env->m_env->m_tdriver, 0); */

    /* test_net_endpoint_assert_buf_memory(cli_ep, net_ep_buf_read, "efgh", 4); */
}

int net_ws_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ws_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
