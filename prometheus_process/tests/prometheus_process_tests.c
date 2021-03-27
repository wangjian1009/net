#include "cmocka_all.h"
#include "prometheus_process_tests.h"

int main(void) {
    int rv = 0;

    if (prometheus_process_limits_basic_tests() != 0) rv = -1;

    if (prometheus_process_collector_limits_tests() != 0) rv = -1;
    if (prometheus_process_collector_stat_tests() != 0) rv = -1;

    return rv;
}

