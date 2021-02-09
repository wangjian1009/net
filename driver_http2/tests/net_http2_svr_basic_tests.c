#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
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

static void net_http2_svr_basic(void **state) {
    net_http2_testenv_t env = *state;

    net_http2_endpoint_t endpoint = net_http2_testenv_svr_ep_create(env);
    assert_true(endpoint);

    net_endpoint_t base_endpoint = net_http2_endpoint_base_endpoint(endpoint);
    
    net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
}

int net_http2_svr_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_http2_svr_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
