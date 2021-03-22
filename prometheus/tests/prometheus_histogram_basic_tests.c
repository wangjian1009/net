#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_histogram.h"
#include "prometheus_histogram_buckets.h"
#include "prometheus_metric.h"
#include "prometheus_metric_sample.h"
#include "prometheus_metric_sample_histogram.h"

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

static void test_histogram_basic(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_histogram_t h =
        prometheus_histogram_create(
            env->m_manager, "test_histogram", "histogram under test",
            prometheus_histogram_buckets_linear(env->m_manager, 5.0, 5.0, 3), 0, NULL);

    prometheus_histogram_observe(h, 1.0, NULL);
    prometheus_histogram_observe(h, 7.0, NULL);
    prometheus_histogram_observe(h, 11.0, NULL);
    prometheus_histogram_observe(h, 22.0, NULL);

    prometheus_metric_sample_histogram_t h_sample =
        prometheus_metric_sample_histogram_from_labels(prometheus_metric_from_data(h), NULL);

    // Test counter for each bucket

    prometheus_metric_sample_t sample;
    
    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram{le=\"5.0\"}");
    assert_true(sample != NULL);
    assert_float_equal(1.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram{le=\"10.0\"}");
    assert_true(sample != NULL);
    assert_float_equal(2.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram{le=\"15.0\"}");
    assert_true(sample != NULL);
    assert_float_equal(3.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram{le=\"+Inf\"}");
    assert_true(sample != NULL);
    assert_float_equal(4.0, prometheus_metric_sample_r_value(sample), 0.0001);
    
    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram_count");
    assert_true(sample != NULL);
    assert_float_equal(4.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_histogram_find_sample_by_l_value(h_sample, "test_histogram_sum");
    assert_true(sample != NULL);
    assert_float_equal(41.0, prometheus_metric_sample_r_value(sample), 0.0001);

    prometheus_histogram_free(h);
}

int prometheus_histogram_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_histogram_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
