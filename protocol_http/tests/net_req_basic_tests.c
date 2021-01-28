#include "cmocka_all.h"
#include "net_http_testenv.h"
#include "net_http_tests.h"

static int setup(void **state) {
    net_http_testenv_t env = net_http_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_http_testenv_t env = *state;
    net_http_testenv_free(env);
    return 0;
}

static void http_req_basic(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    //net_http_req_t req = net_http_req_
}

int net_req_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(http_req_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
