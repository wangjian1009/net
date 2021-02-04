#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_testenv.h"
#include "net_ssl_stream_endpoint.h"
#include "test_net_endpoint.h"

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

static void net_ssl_svr_undline_established(void **state) {
    net_ssl_testenv_t env = *state;
    net_endpoint_t ssl_endpoint = net_ssl_testenv_create_stream_endpoint(env);
    net_endpoint_t underline = net_ssl_stream_endpoint_underline(ssl_endpoint);
    assert_true(underline != NULL);
    
    assert_true(net_endpoint_set_state(underline, net_endpoint_state_established) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ssl_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

int net_ssl_stream_svr_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_svr_undline_established, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
