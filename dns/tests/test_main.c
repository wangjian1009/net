#include "test_dns_manager.h"

int main(void) {
    int n;
    SRunner* sr;
    sr = srunner_create(dns_records_suite());
    srunner_run_all(sr, CK_NORMAL);
    n = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (n == 0) ? 0 : -1;
}
