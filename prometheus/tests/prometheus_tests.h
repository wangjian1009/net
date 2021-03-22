#ifndef TESTS_PROMETHEUS_TESTS_H_INCLEDED
#define TESTS_PROMETHEUS_TESTS_H_INCLEDED
#include "prometheus_types.h"

int prometheus_manager_basic_tests();
int prometheus_collector_basic_tests();

int prometheus_counter_basic_tests();
int prometheus_gauge_basic_tests();

int prometheus_histogram_buckets_basic_tests();

#endif
