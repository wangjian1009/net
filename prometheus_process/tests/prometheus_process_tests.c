#include "cmocka_all.h"
#include "prometheus_process_tests.h"

int main(void) {
    CPE_BEGIN_RUN_TESTS()
        CPE_ADD_TEST_SUIT(prometheus_process_linux_limits_basic_tests),
        CPE_ADD_TEST_SUIT(prometheus_process_linux_collector_limits_tests),
        CPE_ADD_TEST_SUIT(prometheus_process_linux_collector_stat_tests),
        CPE_ADD_TEST_SUIT(prometheus_process_linux_collector_basic_tests),
    CPE_END_RUN_TESTS();
}

