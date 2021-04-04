#include "cmocka_all.h"
#include "net_http_tests.h"

int main(void) {
    int rv = 0;

    if (net_http_output_basic_tests() != 0) rv = -1;
    if (net_http_output_method_get_tests() != 0) rv = -1;
    if (net_http_output_method_head_tests() != 0) rv = -1;

    if (net_http_input_basic_tests() != 0) rv = -1;

    return rv;
}

