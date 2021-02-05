#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "test_net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_testenv.h"
#include "net_ssl_stream_endpoint.h"

static int setup(void **state) {
    net_ssl_testenv_t env = net_ssl_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_testenv_free(env);
    return 0;
}

static void net_ssl_stream_cli_connect_success(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream =
        net_ssl_testenv_create_stream_cli_endpoint(env, "1.2.3.4:5678");
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
    
    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_success(cli_ssl_base, "1.2.3.4:5678", 0);
    test_net_endpoint_expect_write_keep(cli_ssl_base, net_ep_buf_user1);

    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    assert_int_equal(net_endpoint_error_no(cli_stream_base), 0);
}

static void net_ssl_stream_cli_connect_success_delay(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env, "1.2.3.4:5678");
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_success(cli_ssl_base, "1.2.3.4:5678", 100);

    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_endpoint_expect_write_keep(cli_ssl_base, net_ep_buf_user1);
    test_net_driver_run(env->m_tdriver, 100);
    
    assert_int_equal(net_endpoint_error_no(cli_stream_base), 0);
}

static void net_ssl_stream_cli_connect_error(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env, "1.2.3.4:5678");
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);

    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_error(
        cli_ssl_base, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        0);

    assert_true(net_endpoint_connect(cli_stream_base) != 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(cli_stream_base)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(cli_stream_base),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(cli_stream_base),
        "error1");
}

static void net_ssl_stream_cli_connect_error_delay(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_stream_endpoint_t cli_stream = net_ssl_testenv_create_stream_cli_endpoint(env, "1.2.3.4:5678");
    net_endpoint_t cli_stream_base = net_ssl_stream_endpoint_base_endpoint(cli_stream);
        
    net_ssl_endpoint_t cli_ssl = net_ssl_stream_endpoint_underline(cli_stream);
    assert_true(cli_ssl != NULL);

    net_endpoint_t cli_ssl_base = net_ssl_endpoint_base_endpoint(cli_ssl);
    
    test_net_endpoint_expect_connect_error(
        cli_ssl_base, "1.2.3.4:5678",
        net_endpoint_error_source_network, net_endpoint_network_errno_network_error, "error1",
        100);

    assert_true(net_endpoint_connect(cli_stream_base) == 0);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_connecting));

    test_net_driver_run(env->m_tdriver, 100);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(cli_stream_base)),
        net_endpoint_state_str(net_endpoint_state_error));

    assert_string_equal(
        net_endpoint_error_source_str(net_endpoint_error_source(cli_stream_base)),
        net_endpoint_error_source_str(net_endpoint_error_source_network));
    
    assert_int_equal(
        net_endpoint_error_no(cli_stream_base),
        net_endpoint_network_errno_network_error);

    assert_string_equal(
        net_endpoint_error_msg(cli_stream_base),
        "error1");
}

int net_ssl_stream_cli_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_stream_cli_connect_success, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_cli_connect_success_delay, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_cli_connect_error, setup, teardown),
		cmocka_unit_test_setup_teardown(net_ssl_stream_cli_connect_error_delay, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
