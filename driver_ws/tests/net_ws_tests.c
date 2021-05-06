#include "cmocka_all.h"
#include "net_ws_tests.h"

int main(void) {
    CPE_BEGIN_RUN_TESTS()
        /*basic*/
        CPE_ADD_TEST_SUIT(net_ws_svr_basic_tests),
        CPE_ADD_TEST_SUIT(net_ws_pair_basic_tests),
    
        /*stream*/
        CPE_ADD_TEST_SUIT(net_ws_stream_cli_basic_tests),
        /* CPE_ADD_TEST_SUIT(net_ws_stream_svr_basic_tests), */
        /* CPE_ADD_TEST_SUIT(net_ws_stream_pair_basic_tests), */

    CPE_END_RUN_TESTS();
}


