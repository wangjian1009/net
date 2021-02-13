#ifndef TESTS_NET_HTTP2_TESTS_H_INCLEDED
#define TESTS_NET_HTTP2_TESTS_H_INCLEDED
#include "net_http2_types.h"

/*basic*/
int net_http2_basic_pair_basic_tests();

/*stream*/
int net_http2_stream_cli_basic_tests();
int net_http2_stream_svr_basic_tests();
int net_http2_stream_pair_basic_tests();

#endif

