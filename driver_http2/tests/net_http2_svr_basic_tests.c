#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_http2_tests.h"
#include "net_http2_testenv.h"
#include "test_net_endpoint.h"

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

static void net_http2_svr_control_established(void **state) {
    net_http2_testenv_t env = *state;
    net_http2_endpoint_t endpoint = net_http2_testenv_create_ep(env);
    net_endpoint_t base_endpoint = net_http2_endpoint_base_endpoint(endpoint);
    
    net_http2_control_endpoint_t control = net_http2_endpoint_control(endpoint);
    assert_true(control != NULL);
    net_endpoint_t base_control = net_http2_control_endpoint_base_endpoint(control);
    
    assert_true(net_endpoint_set_state(base_control, net_endpoint_state_established) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(base_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

int net_http2_svr_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_svr_control_established, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
