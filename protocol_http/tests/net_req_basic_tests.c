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

static void http_init(void **state) {
    net_http_testenv_t env = *state;
}

int net_req_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(http_init, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
