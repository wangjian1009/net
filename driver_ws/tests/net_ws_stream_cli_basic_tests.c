#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"
#include "net_ws_cli_stream_endpoint.h"
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

static void net_ws_stream_cli_underline_connecting(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t stream_endpoint = net_ws_testenv_cli_stream_ep_create(env);
    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(stream_endpoint);

    assert_true(net_endpoint_set_state(underline, net_endpoint_state_connecting) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(stream_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

static void net_ws_stream_cli_underline_established(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t stream_endpoint = net_ws_testenv_cli_stream_ep_create(env);
    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(stream_endpoint);

    test_net_endpoint_expect_write_keep(underline, net_ep_buf_user1);
    assert_true(net_endpoint_set_state(underline, net_endpoint_state_established) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(stream_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

static void net_ws_cli_connect_success(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t ep = net_ws_testenv_cli_stream_ep_create(env);

    net_address_t target_addr = net_address_create_auto(env->m_schedule, "1.2.3.4:5678");
    net_endpoint_set_remote_address(ep, target_addr);
    net_address_free(target_addr);

    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(ep);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_success(underline, "1.2.3.4:5678", 0);
    test_net_endpoint_expect_write_keep(underline, net_ep_buf_user1);

    assert_true(net_endpoint_connect(ep) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_int_equal(net_endpoint_error_no(ep), 0);
}

static void net_ws_cli_connect_success_delay(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t ep = net_ws_testenv_cli_stream_ep_create(env);

    net_address_t target_addr = net_address_create_auto(env->m_schedule, "1.2.3.4:5678");
    net_endpoint_set_remote_address(ep, target_addr);
    net_address_free(target_addr);

    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(ep);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_success(underline, "1.2.3.4:5678", 100);

    assert_true(net_endpoint_connect(ep) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_endpoint_expect_write_keep(underline, net_ep_buf_user1);
    test_net_driver_run(env->m_tdriver, 100);
    
    assert_int_equal(net_endpoint_error_no(ep), 0);
}

static void net_ws_cli_connect_error(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t ep = net_ws_testenv_cli_stream_ep_create(env);

    net_address_t target_addr = net_address_create_auto(env->m_schedule, "1.2.3.4:5678");
    net_endpoint_set_remote_address(ep, target_addr);
    net_address_free(target_addr);

    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(ep);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_error(
        underline, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        0);

    assert_true(net_endpoint_connect(ep) != 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(ep)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(ep),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(ep),
        "error1");
}

static void net_ws_cli_connect_error_delay(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t ep = net_ws_testenv_cli_stream_ep_create(env);

    net_address_t target_addr = net_address_create_auto(env->m_schedule, "1.2.3.4:5678");
    net_endpoint_set_remote_address(ep, target_addr);
    net_address_free(target_addr);

    net_endpoint_t underline = net_ws_cli_stream_endpoint_underline(ep);
    assert_true(underline != NULL);

    test_net_endpoint_expect_connect_error(
        underline, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        100);

    assert_true(net_endpoint_connect(ep) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_tdriver, 100);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(ep)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(ep),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(ep),
        "error1");
}

int net_ws_stream_cli_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ws_stream_cli_underline_connecting, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_stream_cli_underline_established, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_cli_connect_success, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_cli_connect_success_delay, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_cli_connect_error, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ws_cli_connect_error_delay, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
