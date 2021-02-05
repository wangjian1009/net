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

static void net_ssl_svr_basic(void **state) {
    net_ssl_testenv_t env = *state;
    net_ssl_stream_endpoint_t svr_stream = net_ssl_testenv_create_stream_svr_endpoint(env);
    net_ssl_endpoint_t svr_ssl = net_ssl_stream_endpoint_underline(svr_stream);
    assert_true(svr_ssl != NULL);
}

int net_ssl_stream_svr_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_svr_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
