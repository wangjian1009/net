#ifndef TESTS_PROMETHEUS_PROCESS_TESTS_H_INCLEDED
#define TESTS_PROMETHEUS_PROCESS_TESTS_H_INCLEDED
#include "prometheus_process_types.h"

int prometheus_process_linux_limits_basic_tests();
int prometheus_process_linux_collector_limits_tests();
int prometheus_process_linux_collector_stat_tests();

#endif
