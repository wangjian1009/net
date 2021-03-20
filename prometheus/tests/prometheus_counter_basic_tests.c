#include "cmocka_all.h"
#include "prometheus_tests.h"
#include "prometheus_testenv.h"
#include "prometheus_counter.h"

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

const char *sample_labels_a[] = {"f", "b"};
const char *sample_labels_b[] = {"o", "r"};

static void test_counter_inc(void **state) {
    prometheus_testenv_t env = *state;

    prometheus_counter_t c =
        prometheus_counter_create(
            env->m_manager, "test_counter", "counter under test",
        2, (const char *[]){"foo", "bar"});
    assert_true(c);

    /* prom_counter_inc(c, sample_labels_a); */

    /* prom_metric_sample_t *sample = prom_metric_sample_from_labels(c, sample_labels_a); */
    /* TEST_ASSERT_EQUAL_DOUBLE(1.0, sample->r_value); */

    /* sample = prom_metric_sample_from_labels(c, sample_labels_b); */
    /* TEST_ASSERT_EQUAL_DOUBLE(0.0, sample->r_value); */

    prometheus_counter_free(c);
    c = NULL;
}

static void test_counter_add(void **state) {
    prometheus_testenv_t env = *state;

    prometheus_counter_t c =
        prometheus_counter_create(
            env->m_manager,
            "test_counter", "counter under test",
            2, (const char *[]){"foo", "bar"});
    assert_true(c);

    prometheus_counter_add(c, 100000000.1, sample_labels_a);
    /* prom_metric_sample_t *sample = prom_metric_sample_from_labels(c, sample_labels_a); */
    /* TEST_ASSERT_EQUAL_DOUBLE(100000000.1, sample->r_value); */

    /* sample = prom_metric_sample_from_labels(c, sample_labels_b); */
    /* TEST_ASSERT_EQUAL_DOUBLE(0.0, sample->r_value); */

    prometheus_counter_free(c);
    c = NULL;
}

int prometheus_counter_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_counter_inc, setup, teardown),
		cmocka_unit_test_setup_teardown(test_counter_add, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
