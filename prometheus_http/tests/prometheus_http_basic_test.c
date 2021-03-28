#include "cmocka_all.h"
#include "prometheus_http_tests.h"
#include "prometheus_http_testenv.h"

static int setup(void **state) {
    prometheus_http_testenv_t env = prometheus_http_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_http_testenv_t env = *state;
    prometheus_http_testenv_free(env);
    return 0;
}

static void test_get(void **state) {
    prometheus_http_testenv_t env = *state;
}

int prometheus_http_basic_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_get, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
