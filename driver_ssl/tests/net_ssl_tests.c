#include "cmocka_all.h"
#include "net_ssl_tests.h"

int main(void) {
    int rv = 0;

    if (net_ssl_stream_cli_basic_tests() != 0) rv = -1;
    if (net_ssl_stream_svr_basic_tests() != 0) rv = -1;
    if (net_ssl_stream_pair_basic_tests() != 0) rv = -1;
    if (net_ssl_stream_pair_big_response_tests() != 0) rv = -1;
    
    return rv;
}


