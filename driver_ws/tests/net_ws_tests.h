#ifndef TESTS_NET_WS_TESTS_H_INCLEDED
#define TESTS_NET_WS_TESTS_H_INCLEDED
#include "cmocka_all.h"
#include "net_ws_types.h"

CPE_DECLARE_TEST_SUIT(net_ws_svr_basic_tests);
CPE_DECLARE_TEST_SUIT(net_ws_pair_basic_tests);

/*stream*/
CPE_DECLARE_TEST_SUIT(net_ws_stream_cli_basic_tests);
CPE_DECLARE_TEST_SUIT(net_ws_stream_svr_basic_tests);
CPE_DECLARE_TEST_SUIT(net_ws_stream_pair_basic_tests);

#endif

