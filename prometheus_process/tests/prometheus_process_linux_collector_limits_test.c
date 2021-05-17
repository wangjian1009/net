#include "cmocka_all.h"
#include "prometheus_collector.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_testenv.h"

static int setup(void **state) {
    prometheus_process_testenv_t env = prometheus_process_testenv_create();
    *state = env;

    prometheus_process_install_linux_limits(
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

static void test_max_fds(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create_linux(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_max_fds(env->m_provider)) == 0);

    assert_string_equal(
        "# HELP process_max_fds Maximum number of open file descriptors.\n"
        "# TYPE process_max_fds gauge\n"
        "process_max_fds 1048576\n",
        prometheus_process_testenv_collect(env));
}

static void test_virtual_memory_max_bytes(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create_linux(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_virtual_memory_max_bytes(env->m_provider)) == 0);

    assert_string_equal(
        "# HELP process_virtual_memory_max_bytes Maximum amount of virtual memory available in bytes.\n"
        "# TYPE process_virtual_memory_max_bytes gauge\n"
        "process_virtual_memory_max_bytes -1\n",
        prometheus_process_testenv_collect(env));
}

CPE_BEGIN_TEST_SUIT(prometheus_process_linux_collector_limits_tests)
    cmocka_unit_test_setup_teardown(test_max_fds, setup, teardown),
    cmocka_unit_test_setup_teardown(test_virtual_memory_max_bytes, setup, teardown),
CPE_END_TEST_SUIT(prometheus_process_linux_collector_limits_tests)
