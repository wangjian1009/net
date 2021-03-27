#include "cpe/pal/pal_unistd.h"
#include "cmocka_all.h"
#include "prometheus_collector.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_testenv.h"

static int setup(void **state) {
    prometheus_process_testenv_t env = prometheus_process_testenv_create();
    *state = env;

    prometheus_process_install_stat(
        env,
        "1 "               /* (1) pid  %d */
        "(bash) "          /* (2) comm  %s */
        "S "               /* (3) state  %c */
        "0 "               /* (4) ppid  %d */
        "1 "               /* (5) pgrp  %d */
        "1 "               /* (6) session  %d */
        "34816 "           /* (7) tty_nr  %d */
        "410 "             /* (8) tpgid  %d */
        "4210944 "         /* (9) flags  %u */
        "1463 "            /* (10) minflt  %lu */
        "89550 "           /* (11) cminflt  %lu */
        "0 "               /* (12) majflt  %lu */
        "7 "               /* (13) cmajflt  %lu */
        "3 "               /* (14) utime  %lu */
        "4 "               /* (15) stime  %lu */
        "165 "             /* (16) cutime  %ld */
        "193 "             /* (17) cstime  %ld */
        "20 "              /* (18) priority  %ld */
        "0 "               /* (19) nice  %ld */
        "1 "               /* (20) num_threads  %ld */
        "0 "               /* (21) itrealvalue  %ld */
        "29414985 "        /* (22) starttime  %llu */
        "19058688 "        /* (23) vsize  %lu */
        "885 "             /* (24) rss  %ld */
        "18446744073709551615 " /* (25) rsslim  %lu */
        "94298705027072 "  /* (26) startcode  %lu  [PT] */
        "94298706087992 "  /* (27) endcode  %lu  [PT] */
        "140736141303504 " /* (28) startstack  %lu  [PT] */
        "0 "               /* (29) kstkesp  %lu  [PT] */
        "0 "               /* (30) kstkeip  %lu  [PT] */
        "0 "               /* (31) signal  %lu */
        "65536 "           /* (32) blocked  %lu */
        "3670020 "         /* (33) sigignore  %lu */
        "1266777851 "      /* (34) sigcatch  %lu */
        "0 "               /* (35) wchan  %lu  [PT] */
        "0 "               /* (36) nswap  %lu */
        "0 "               /* (37) cnswap  %lu */
        "17 "              /* (38) exit_signal  %d  (since Linux 2.1.22) */
        "0 "               /* (39) processor  %d  (since Linux 2.2.8) */
        "0 "               /* (40) rt_priority  %u  (since Linux 2.5.19) */
        "0 "               /* (41) policy  %u  (since Linux 2.5.19) */
        "0 "               /* (42) delayacct_blkio_ticks */
        "0 "               /* (43) guest_time  %lu  (since Linux 2.6.24) */
        "0 "               /* (44) cguest_time  %ld  (since Linux 2.6.24) */
        "94298708188560 "  /* (45) start_data  %lu  (since Linux 3.3)  [PT] */
        "94298708235620 "  /* (46) end_data  %lu  (since Linux 3.3)  [PT] */
        "94298741563392 "  /* (47) start_brk  %lu  (since Linux 3.3)  [PT] */
        "140736141311847 " /* (48) arg_start  %lu  (since Linux 3.5)  [PT] */
        "140736141311857 " /* (49) arg_end  %lu  (since Linux 3.5)  [PT] */
        "140736141311857 " /* (50) env_start  %lu  (since Linux 3.5)  [PT] */
        "140736141311982 " /* (51) env_end  %lu  (since Linux 3.5)  [PT] */
        "0"                /* (52) exit_code  %d  (since Linux 3.5)  [PT] */
        );

    return 0;
}

static int teardown(void **state) {
    prometheus_process_testenv_t env = *state;
    prometheus_process_testenv_free(env);
    return 0;
}

static void test_cpu_seconds_total(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_cpu_seconds_total(env->m_provider)) == 0);

    sysconf(_SC_CLK_TCK);

    assert_string_equal(
        "# HELP process_cpu_seconds_total Total user and system CPU time spent in seconds.\n"
        "# TYPE process_cpu_seconds_total gauge\n"
        "process_cpu_seconds_total 0\n",
        prometheus_process_testenv_collect(env));
}

static void test_virtual_memory_bytes(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_virtual_memory_bytes(env->m_provider)) == 0);

    assert_string_equal(
        "# HELP process_virtual_memory_bytes Virtual memory size in bytes.\n"
        "# TYPE process_virtual_memory_bytes gauge\n"
        "process_virtual_memory_bytes 19058688\n",
        prometheus_process_testenv_collect(env));
}

static void test_resident_memory_bytes(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_resident_memory_bytes(env->m_provider)) == 0);

    char expect[256];
    snprintf(
        expect, sizeof(expect),
        "# HELP process_resident_memory_bytes Resident memory size in bytes.\n"
        "# TYPE process_resident_memory_bytes gauge\n"
        "process_resident_memory_bytes %ld\n",
        885 * sysconf(_SC_PAGE_SIZE));
    
    assert_string_equal(
        expect,
        prometheus_process_testenv_collect(env));
}

static void test_start_time_seconds(void **state) {
    prometheus_process_testenv_t env = *state;

    prometheus_collector_t collector = prometheus_process_collector_create(env->m_provider, "test");
    assert_true(collector);

    assert_true(
        prometheus_collector_add_metric(
            collector, prometheus_process_provider_start_time_seconds(env->m_provider)) == 0);

    assert_string_equal(
        "# HELP process_start_time_seconds Start time of the process since unix epoch in seconds.\n"
        "# TYPE process_start_time_seconds gauge\n"
        "process_start_time_seconds 29414985\n",
        prometheus_process_testenv_collect(env));
}

int prometheus_process_linux_collector_stat_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_cpu_seconds_total, setup, teardown),
        cmocka_unit_test_setup_teardown(test_virtual_memory_bytes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_resident_memory_bytes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_start_time_seconds, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
