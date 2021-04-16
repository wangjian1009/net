#include "cmocka_all.h"
#include "net_core_tests.h"

int main(void) {
    int rv = 0;

    if (net_address_matcher_basic_tests() != 0) rv = -1;

    if (net_ringbuffer_basic_tests() != 0) rv = -1;

    if (net_pair_basic_tests() != 0) rv = -1;
    if (net_pair_established_tests() != 0) rv = -1;

    if (net_progress_basic_tests() != 0) rv = -1;

    return rv;
}

