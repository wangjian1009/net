#include "cmocka_all.h"
#include "prometheus_tests.h"

int main(void) {
    int rv = 0;

    if (prometheus_manager_basic_tests() != 0) rv = -1;
    if (prometheus_collector_basic_tests() != 0) rv = -1;
    if (prometheus_counter_basic_tests() != 0) rv = -1;
    if (prometheus_gauge_basic_tests() != 0) rv = -1;

    if (prometheus_histogram_buckets_basic_tests() != 0) rv = -1;

    return rv;
}

