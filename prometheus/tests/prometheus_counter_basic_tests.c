#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"

static int setup(void **state) {
    prometheus_testenv_t env = prometheus_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_testenv_t env = *state;
    prometheus_testenv_free(env);
    return 0;
}

static void rule_basic(void **state) {
}

int prometheus_counter_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(rule_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
