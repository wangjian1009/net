#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"
#include "net_ws_svr_endpoint.h"
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

static void net_ws_stream_svr_underline_established(void **state) {
    net_ws_testenv_t env = *state;
    net_endpoint_t ws_endpoint = net_ws_testenv_create_svr_endpoint(env);
    net_endpoint_t underline = net_ws_svr_endpoint_underline(ws_endpoint);
    assert_true(underline != NULL);
    
    assert_true(net_endpoint_set_state(underline, net_endpoint_state_established) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ws_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

int net_ws_stream_svr_basic_tests() {
	const struct CMUnitTest ws_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ws_stream_svr_underline_established, setup, teardown),
	};
	return cmocka_run_group_tests(ws_basic_tests, NULL, NULL);
}
