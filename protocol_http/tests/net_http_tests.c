#include "net_http_tests.h"

int main(void) {
    CPE_BEGIN_RUN_TESTS()

        CPE_ADD_TEST_SUIT(net_http_output_basic_tests),
        CPE_ADD_TEST_SUIT(net_http_output_method_get_tests),
        CPE_ADD_TEST_SUIT(net_http_output_method_head_tests),

        CPE_ADD_TEST_SUIT(net_http_input_basic_tests),

        CPE_ADD_TEST_SUIT(net_https_basic_tests),
    
    CPE_END_RUN_TESTS();
}

