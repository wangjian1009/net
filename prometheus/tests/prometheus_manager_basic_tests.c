#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_manager.h"
#include "prometheus_metric.h"

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

static void test_large_registry(void **state) {
    prometheus_testenv_t env = *state;

    for (int i = 0; i < 1000; i++) {
        char metric[6];
        sprintf(metric, "%d", i);
        //prom_collector_registry_must_register_metric(prom_counter_new(metric, metric, 0, NULL));
    }
}

int prometheus_manager_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_large_registry, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
