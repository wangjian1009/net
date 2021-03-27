#include "cmocka_all.h"
#include "prometheus_collector.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_testenv.h"

static int setup(void **state) {
    prometheus_process_testenv_t env = prometheus_process_testenv_create();
    *state = env;

    prometheus_process_install_limits(
        env,
        "Limit                     Soft Limit           Hard Limit           Units\n"
        "Max cpu time              unlimited            unlimited            seconds\n"
        "Max file size             unlimited            unlimited            bytes\n"
        "Max data size             unlimited            unlimited            bytes\n"
        "Max stack size            8388608              unlimited            bytes\n"
        "Max core file size        0                    unlimited            bytes\n"
        "Max resident set          unlimited            unlimited            bytes\n"
        "Max processes             unlimited            unlimited            processes\n"
        "Max open files            1048576              1048576              files\n"
        "Max locked memory         83968000             83968000             bytes\n"
        "Max address space         unlimited            unlimited            bytes\n"
        "Max file locks            unlimited            unlimited            locks\n"
        "Max pending signals       23701                23701                signals\n"
        "Max msgqueue size         819200               819200               bytes\n"
        "Max nice priority         0                    0\n"
        "Max realtime priority     0                    0\n"
        "Max realtime timeout      unlimited            unlimited            us\n"
        );

    return 0;
}

static int teardown(void **state) {
    prometheus_process_testenv_t env = *state;
    prometheus_process_testenv_free(env);
    return 0;
}

static void aa(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_cpu_seconds_total(env->m_provider)) == 0);

    
}

int prometheus_process_collector_limits_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(aa, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
