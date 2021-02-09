#include "cmocka_all.h"
#include "net_http2_tests.h"

int main(void) {
    int rv = 0;

    /*basic*/
    if (net_http2_svr_basic_tests() != 0) rv = -1;
    if (net_http2_pair_basic_tests() != 0) rv = -1;
    
    /*stream*/
    if (net_http2_stream_cli_basic_tests() != 0) rv = -1;
    /* if (net_http2_stream_svr_basic_tests() != 0) rv = -1; */
    /* if (net_http2_stream_pair_basic_tests() != 0) rv = -1; */

    return rv;
}


