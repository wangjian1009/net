#include "prometheus_http_tests.h"

int main(void) {
    CPE_BEGIN_RUN_TESTS()
        CPE_ADD_TEST_SUIT(prometheus_http_basic_tests),
    CPE_END_RUN_TESTS();
}

