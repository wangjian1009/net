#ifndef TESTS_NET_PROTOCOL_HTTP_TESTS_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TESTS_H_INCLEDED
#include "cmocka_all.h"
#include "net_http_protocol.h"

CPE_DECLARE_TEST_SUIT(net_http_output_basic_tests);
CPE_DECLARE_TEST_SUIT(net_http_output_method_get_tests);
CPE_DECLARE_TEST_SUIT(net_http_output_method_head_tests);

CPE_DECLARE_TEST_SUIT(net_http_input_basic_tests);

CPE_DECLARE_TEST_SUIT(net_https_basic_tests);

#endif
