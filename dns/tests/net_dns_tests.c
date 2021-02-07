#include "cmocka_all.h"
#include "net_dns_tests.h"

int main(void) {
    int rv = 0;
    if (net_dns_records_case_basic() != 0) rv = -1;
    if (net_dns_records_case_runtime() != 0) rv = -1;
    return rv;
}



