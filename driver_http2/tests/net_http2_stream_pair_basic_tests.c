#include "cmocka_all.h"
#include "test_net_endpoint.h"
#include "net_http2_tests.h"
#include "net_http2_stream_endpoint.h"
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

static void net_http2_stream_pair_basic(void **state) {
    net_http2_testenv_t env = *state;

    net_acceptor_t svr_acceptor = net_http2_testenv_create_stream_acceptor(env, "1.2.3.4:5678");
    
    net_http2_stream_endpoint_t cli_ep = net_http2_testenv_create_stream_ep_cli(env, "1.2.3.4:5678");
    net_endpoint_t cli_ep_base = net_http2_stream_endpoint_base_endpoint(cli_ep);

    test_net_next_endpoint_expect_connect_to_acceptor(env->m_tdriver, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    net_http2_endpoint_t cli_ctrl = net_http2_stream_endpoint_control(cli_ep);
    assert_true(cli_ctrl);
    net_endpoint_t cli_ctrl_base = net_http2_endpoint_base_endpoint(cli_ctrl);
    
    /*验证状态*/
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ctrl_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    /*等待连接成功 */
    test_net_driver_run(env->m_tdriver, 100);

    /*验证连接成功状态 */
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    net_http2_stream_endpoint_t svr_ep = net_http2_testenv_stream_ep_other(env, cli_ep);
    CPE_ERROR(env->m_em, "aaa 1: %p", svr_ep);
    assert_true(svr_ep != NULL);
    net_endpoint_t svr_ep_base = net_http2_stream_endpoint_base_endpoint(svr_ep);

    /*验证数据发送 */
    /*client -> server*/
    /* assert_true(net_endpoint_buf_append(cli_ep_base, net_ep_buf_write, "abcd", 4) == 0); */
    /* test_net_driver_run(env->m_env->m_tdriver, 0); */

    /* test_net_endpoint_assert_buf_memory(svr_ep, net_ep_buf_read, "abcd", 4); */

    /*server -> client*/
    /* assert_true(net_endpoint_buf_append(svr_ep, net_ep_buf_write, "efgh", 4) == 0); */
    /* test_net_driver_run(env->m_env->m_tdriver, 0); */

    /* test_net_endpoint_assert_buf_memory(cli_ep_base, net_ep_buf_read, "efgh", 4); */
}

int net_http2_stream_pair_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_stream_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
