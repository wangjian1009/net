#include "cmocka_all.h"
#include "prometheus_http_tests.h"

int main(void) {
    int rv = 0;

    if (prometheus_http_basic_tests() != 0) rv = -1;

    return rv;
}

