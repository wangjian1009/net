#include "cmocka_all.h"
#include "net_http_tests.h"

int main(void) {
    int rv = 0;

    if (net_req_basic_tests() != 0) rv = -1;

    return rv;
}

