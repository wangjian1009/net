#include "cmocka_all.h"
#include "net_ws_tests.h"

int main(void) {
    int rv = 0;

    /*basic*/
    
    /*stream*/
    if (net_ws_stream_cli_basic_tests() != 0) rv = -1;
    if (net_ws_stream_svr_basic_tests() != 0) rv = -1;
    if (net_ws_stream_pair_basic_tests() != 0) rv = -1;

    return rv;
}


