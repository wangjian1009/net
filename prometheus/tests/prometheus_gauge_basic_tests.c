#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_gauge.h"
#include "prometheus_metric.h"
#include "prometheus_metric_sample.h"

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

static const char *sample_labels_a[] = {"f", "b"};
static const char *sample_labels_b[] = {"o", "r"};

static void test_gauge_inc(void **state) {
    prometheus_testenv_t env = *state;

    prometheus_gauge_t c =
        prometheus_gauge_create(
            env->m_manager, "test_gauge", "gauge under test",
        2, (const char *[]){"foo", "bar"});
    assert_true(c);

    prometheus_gauge_inc(c, sample_labels_a);

    prometheus_metric_sample_t sample =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(c), sample_labels_a);
    assert_float_equal(1.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_from_labels(prometheus_metric_from_data(c), sample_labels_b);
    assert_float_equal(0.0, prometheus_metric_sample_r_value(sample), 0.0001);

    prometheus_gauge_free(c);
}

static void test_gauge_dec(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_gauge_t g = prometheus_gauge_create(
        env->m_manager, "test_gauge", "gauge under test", 2,
        (const char *[]){"foo", "bar"});
    assert_true(g);

    prometheus_gauge_dec(g, sample_labels_a);

    prometheus_metric_sample_t sample =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_a);
    assert_float_equal(-1.0, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_b);
    assert_float_equal(0.0, prometheus_metric_sample_r_value(sample), 0.0001);

    prometheus_gauge_free(g);
}

static void test_gauge_add(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_gauge_t g = prometheus_gauge_create(
        env->m_manager, "test_gauge", "gauge under test", 2,
        (const char *[]){"foo", "bar"});
    assert_true(g);

    prometheus_gauge_add(g, 100000000.1, sample_labels_a);

    prometheus_metric_sample_t sample =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_a);
    assert_float_equal(100000000.1, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_b);
    assert_float_equal(0.0, prometheus_metric_sample_r_value(sample), 0.0001); 

    prometheus_gauge_free(g);
}

static void test_gauge_sub(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_gauge_t g = prometheus_gauge_create(
        env->m_manager, "test_gauge", "gauge under test", 2,
        (const char *[]){"foo", "bar"});
    assert_true(g);

    prometheus_gauge_sub(g, 100000000.1, sample_labels_a);

    prometheus_metric_sample_t sample =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_a);
    assert_float_equal(-100000000.1, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_b);
    assert_float_equal(0.0, prometheus_metric_sample_r_value(sample), 0.0001);

    prometheus_gauge_free(g);
}

static void test_gauge_set(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_gauge_t g = prometheus_gauge_create(
        env->m_manager, "test_gauge", "gauge under test", 2,
        (const char *[]){"foo", "bar"});
    assert_true(g);

    prometheus_gauge_set(g, 100000000.1, sample_labels_a);

    prometheus_metric_sample_t sample =
        prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_a);
    assert_float_equal(100000000.1, prometheus_metric_sample_r_value(sample), 0.0001);

    sample = prometheus_metric_sample_from_labels(prometheus_metric_from_data(g), sample_labels_b);
    assert_float_equal(0.0, prometheus_metric_sample_r_value(sample), 0.0001);

    prometheus_gauge_free(g);
}

int prometheus_gauge_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_gauge_inc, setup, teardown),
		cmocka_unit_test_setup_teardown(test_gauge_dec, setup, teardown),
		cmocka_unit_test_setup_teardown(test_gauge_add, setup, teardown),
		cmocka_unit_test_setup_teardown(test_gauge_sub, setup, teardown),
		cmocka_unit_test_setup_teardown(test_gauge_set, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
