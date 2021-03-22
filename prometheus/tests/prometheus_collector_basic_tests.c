#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/buffer.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_collector.h"
#include "prometheus_metric.h"
#include "prometheus_counter.h"

static int setup(void **state) {
    struct prometheus_testenv * env = prometheus_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    struct prometheus_testenv * env = *state;
    prometheus_testenv_free(env);
    return 0;
}

static void test_collector_basic(void **state) {
    struct prometheus_testenv * env = *state;

    prometheus_collector_t collector =
        prometheus_collector_create(
            env->m_manager, "test",
            0, NULL, NULL, NULL);
    prometheus_counter_t counter = prometheus_counter_create(env->m_manager, "test_counter", "counter under test", 0, NULL);
    prometheus_collector_add_metric(collector, prometheus_metric_from_data(counter));
    prometheus_collector_free(collector);
}

int prometheus_collector_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_collector_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
