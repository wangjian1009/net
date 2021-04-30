#include "net_ndt7_tests.h"

int main(void) {
    CPE_BEGIN_RUN_TESTS()
        CPE_ADD_TEST_SUIT(net_nd7_tester_basic_tests),
    CPE_END_RUN_TESTS();
}
