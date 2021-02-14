#include <assert.h>
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

static void net_http2_basic_pair_basic(void **state) {
    net_http2_testenv_t env = *state;

    net_acceptor_t svr_acceptor = net_http2_testenv_create_basic_acceptor(env, "1.2.3.4:5678");
    
    net_http2_endpoint_t cli_ep = net_http2_testenv_create_basic_ep_cli(env, "1.2.3.4:5678");
    net_endpoint_t cli_ep_base = net_http2_endpoint_base_endpoint(cli_ep);

    test_net_endpoint_expect_connect_to_acceptor(cli_ep_base, "1.2.3.4:5678", 100, 0);
    
    assert_true(net_endpoint_connect(cli_ep_base) == 0);

    /*验证状态*/
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_string_equal(
        net_http2_endpoint_state_str(net_http2_endpoint_state(cli_ep)),
        net_http2_endpoint_state_str(net_http2_endpoint_state_init));
    
    /*等待连接成功 */
    test_net_driver_run(env->m_tdriver, 100);

    /*验证连接成功状态 */
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_ep_base)),
        net_endpoint_state_str(net_endpoint_state_established));

    assert_string_equal(
        net_http2_endpoint_state_str(net_http2_endpoint_state(cli_ep)),
        net_http2_endpoint_state_str(net_http2_endpoint_state_streaming));

    /**/
    net_http2_req_t req = net_http2_req_create(cli_ep, net_http2_req_method_get, "/a/b");
    assert_true(req);

    assert_true(net_http2_req_start(req) == 0);

    net_http2_testenv_response_t response = net_http2_testenv_req_commit(env, req);
}

int net_http2_basic_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_basic_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
