#ifndef TESTS_NET_SSL_TESTS_H_INCLEDED
#define TESTS_NET_SSL_TESTS_H_INCLEDED
#include "net_ssl_types.h"

int net_ssl_stream_cli_basic_tests();
int net_ssl_stream_svr_basic_tests();
int net_ssl_stream_pair_basic_tests();
int net_ssl_stream_pair_big_response_tests();

#endif

