#include "cmocka_all.h"
#include "net_core_tests.h"

int main(void) {
    int rv = 0;

    if (net_core_address_matcher_basic_tests() != 0) rv = -1;

    return rv;
}

