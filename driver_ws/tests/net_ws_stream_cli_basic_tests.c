#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"
#include "net_ws_stream_endpoint.h"
#include "test_net_endpoint.h"

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

static void net_ws_stream_pair_connect_success(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_stream_endpoint_t ep = net_ws_testenv_stream_cli_ep_create(env, "1.2.3.4:5678", "/a/b");
    net_endpoint_t ep_base = net_ws_stream_endpoint_base_endpoint(ep);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(ep_base);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_success(underline, "1.2.3.4:5678", 0);

    assert_true(net_endpoint_connect(ep_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_int_equal(net_endpoint_error_no(ep_base), 0);
}

static void net_ws_stream_pair_connect_success_delay(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_stream_endpoint_t ep = net_ws_testenv_stream_cli_ep_create(env, "1.2.3.4:5678", "/a/b");
    net_endpoint_t ep_base = net_ws_stream_endpoint_base_endpoint(ep);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(ep_base);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_success(underline, "1.2.3.4:5678", 100);

    assert_true(net_endpoint_connect(ep_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_tdriver, 100);
    
    assert_int_equal(net_endpoint_error_no(ep_base), 0);
}

static void net_ws_connect_error(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_stream_endpoint_t ep = net_ws_testenv_stream_cli_ep_create(env, "1.2.3.4:5678", "/a/b");
    net_endpoint_t ep_base = net_ws_stream_endpoint_base_endpoint(ep);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(ep_base);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_error(
        underline, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        0);

    assert_true(net_endpoint_connect(ep_base) != 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep_base)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(ep_base)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(ep_base),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(ep_base),
        "error1");
}

static void net_ws_connect_error_delay(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_stream_endpoint_t ep = net_ws_testenv_stream_cli_ep_create(env, "1.2.3.4:5678", "/a/b");
    net_endpoint_t ep_base = net_ws_stream_endpoint_base_endpoint(ep);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(ep_base);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_error(
        underline, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        100);

    assert_true(net_endpoint_connect(ep_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_tdriver, 100);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep_base)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(ep_base)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(ep_base),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(ep_base),
        "error1");
}

int net_ws_stream_cli_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ws_stream_pair_connect_success, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_stream_pair_connect_success_delay, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_connect_error, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_connect_error_delay, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
