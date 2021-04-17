#include "cmocka_all.h"
#include "net_driver_sock_tests.h"

int main(void) {
    int rv = 0;

    if (net_driver_sock_progress_basic_tests() != 0) rv = -1;

    return rv;
}

