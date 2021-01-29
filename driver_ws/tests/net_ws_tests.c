#include "cmocka_all.h"
#include "net_ws_tests.h"

int main(void) {
    int rv = 0;

    if (net_ws_cli_basic_tests() != 0) rv = -1;

    if (net_ws_svr_basic_tests() != 0) rv = -1;
    
    if (net_ws_pair_basic_tests() != 0) rv = -1;

    return rv;
}


