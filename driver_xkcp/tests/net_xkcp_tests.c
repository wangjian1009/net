#include "cmocka_all.h"
#include "net_xkcp_tests.h"

int main(void) {
    int rv = 0;

    if (net_xkcp_pair_basic_tests() != 0) rv = -1;

    return rv;
}


