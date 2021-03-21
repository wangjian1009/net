#include "cmocka_all.h"
#include "cpe/utils/buffer.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_manager.h"
#include "prometheus_collector.h"
#include "prometheus_metric.h"
#include "prometheus_metric_sample.h"
#include "prometheus_counter.h"
#include "prometheus_gauge.h"

struct prometheus_manager_test_env {
    prometheus_testenv_t m_env;
    prometheus_counter_t  m_test_counter;;
    prometheus_gauge_t m_test_gauge;
    //prometheus_histogram_t m_test_histogram;
};

static int setup(void **state) {
    struct prometheus_manager_test_env * env = mem_alloc(test_allocrator(), sizeof(struct prometheus_manager_test_env));
    *state = env;

    env->m_env = prometheus_testenv_create();
    const char *label[] = {"label"};
    env->m_test_counter =
        prometheus_counter_create(
            env->m_env->m_manager,
            "test_counter", "counter under test", 1, label);
    prometheus_collector_add_metric(
        prometheus_collector_default(env->m_env->m_manager),
        prometheus_metric_from_data(env->m_test_counter));

    env->m_test_gauge =
        prometheus_gauge_create(
            env->m_env->m_manager,
            "test_gauge", "gauge under test", 1, label);
    prometheus_collector_add_metric(
        prometheus_collector_default(env->m_env->m_manager),
        prometheus_metric_from_data(env->m_test_gauge));
    
    /* test_histogram = prom_collector_registry_must_register_metric(prom_histogram_new( */
    /*   "test_histogram", "histogram under test", prom_histogram_buckets_linear(5.0, 5.0, 2), 0, NULL)); */

  return 0;
}

static int teardown(void **state) {
    struct prometheus_manager_test_env * env = *state;
    prometheus_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
    return 0;
}

static void test_large_registry(void **state) {
    struct prometheus_manager_test_env * env = *state;

    for (int i = 0; i < 1000; i++) {
        char metric[6];
        sprintf(metric, "%d", i);
        assert_true(prometheus_counter_create(env->m_env->m_manager, metric, metric, 0, NULL) != NULL);
    }
}

static void test_must_register(void **state) {
    struct prometheus_manager_test_env * env = *state;

    int r = 0;

    const char *labels[] = {"foo"};
    assert_true(prometheus_counter_inc(env->m_test_counter, labels) == 0);
    assert_true(prometheus_gauge_add(env->m_test_gauge, 2.0, labels) == 0);

    prometheus_metric_sample_t test_sample_a =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(env->m_test_counter), labels);
    
    prometheus_metric_sample_t test_sample_b =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(env->m_test_gauge), labels);

    assert_float_equal(1.0, prometheus_metric_sample_r_value(test_sample_a), 0.001);
    assert_float_equal(2.0, prometheus_metric_sample_r_value(test_sample_b), 0.001);
}

void test_dump(void ** state) {
    struct prometheus_manager_test_env * env = *state;
    
    const char *labels[] = {"foo"};
    prometheus_counter_inc(env->m_test_counter, labels);
    prometheus_gauge_set(env->m_test_gauge, 2.0, labels);

    /* r = prom_histogram_observe(test_histogram, 3.0, NULL); */
    /* r = prom_histogram_observe(test_histogram, 7.0, NULL); */

    struct mem_buffer data_buffer;
    mem_buffer_init(&data_buffer, test_allocrator());

    assert_string_equal(
        prometheus_manager_collect_dump(&data_buffer, env->m_env->m_manager),
        "# HELP test_counter counter under test\n"
        "# TYPE test_counter counter\n"
        "test_counter{label=\"foo\"}\n"
        "HELP test_gauge gauge under test\n"
        "# TYPE test_gauge gauge\n"
        "test_gauge{label=\"foo\"}\n"
        "# HELP test_histogram histogram under test\n"
        "# TYPE test_histogram histogram\ntest_histogram{le=\"5.0\"}\n"
        "test_histogram{le=\"10.0\"}\n"
        "test_histogram{le=\"+Inf\"}\n"
        "test_histogram_count\n"
        "test_histogram_sum\n"
        );

    mem_buffer_clear(&data_buffer);
}

int prometheus_manager_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_large_registry, setup, teardown),
		cmocka_unit_test_setup_teardown(test_must_register, setup, teardown),
		cmocka_unit_test_setup_teardown(test_dump, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
