#include "cmocka_all.h"
#include "prometheus_collector.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_testenv.h"

static int setup(void **state) {
    prometheus_process_testenv_t env = prometheus_process_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_process_testenv_t env = *state;
    prometheus_process_testenv_free(env);
    return 0;
}

static void test_open_fds(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_process_install_linux_fds(env, 2);

    prometheus_collector_t collector = prometheus_process_collector_create_linux(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_open_fds(env->m_provider)) == 0);

    assert_string_equal(
        "# HELP process_open_fds Number of open file descriptors.\n"
        "# TYPE process_open_fds gauge\n"
        "process_open_fds 2\n",
        prometheus_process_testenv_collect(env));
}

int prometheus_process_linux_collector_basic_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_open_fds, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
