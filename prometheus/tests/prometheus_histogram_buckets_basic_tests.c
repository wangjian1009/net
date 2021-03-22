#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_histogram_buckets.h"

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

static void test_histogram_buckets_new(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_histogram_buckets_t result =
        prometheus_histogram_buckets_create(env->m_manager, 3, 1.0, 2.0, 3.0);
    assert_int_equal(3, prometheus_histogram_buckets_count(result));
    assert_float_equal(1.0, prometheus_histogram_buckets_upper_bound(result, 0), 0.0001);
    assert_float_equal(2.0, prometheus_histogram_buckets_upper_bound(result, 1), 0.0001);
    assert_float_equal(3.0, prometheus_histogram_buckets_upper_bound(result, 2), 0.0001);

    prometheus_histogram_buckets_free(result);
}

static void test_histogram_buckets_linear(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_histogram_buckets_t result =
        prometheus_histogram_buckets_linear(env->m_manager, 0.0, 1.5, 3);

    assert_int_equal(3, prometheus_histogram_buckets_count(result));
    assert_float_equal(0.0, prometheus_histogram_buckets_upper_bound(result, 0), 0.0001);
    assert_float_equal(1.5, prometheus_histogram_buckets_upper_bound(result, 1), 0.0001);
    assert_float_equal(3.0, prometheus_histogram_buckets_upper_bound(result, 2), 0.0001);

    prometheus_histogram_buckets_free(result);
}

static void test_histogram_buckets_expontential(void ** state) {
    prometheus_testenv_t env = *state;

    prometheus_histogram_buckets_t result =
        prometheus_histogram_buckets_exponential(env->m_manager, 1, 2, 3);

    assert_int_equal(3, prometheus_histogram_buckets_count(result));
    assert_float_equal(1.0, prometheus_histogram_buckets_upper_bound(result, 0), 0.0001);
    assert_float_equal(2.0, prometheus_histogram_buckets_upper_bound(result, 1), 0.0001);
    assert_float_equal(4.0, prometheus_histogram_buckets_upper_bound(result, 2), 0.0001);

    prometheus_histogram_buckets_free(result);
}

int prometheus_histogram_buckets_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_histogram_buckets_new, setup, teardown),
		cmocka_unit_test_setup_teardown(test_histogram_buckets_linear, setup, teardown),
		cmocka_unit_test_setup_teardown(test_histogram_buckets_expontential, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
