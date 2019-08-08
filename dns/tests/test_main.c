#include "test_dns_manager.h"

int main(void) {
    int n;
    SRunner* sr;
    
    Suite* suite_records = suite_create("dns_records");

    suite_add_tcase(suite_records, dns_records_case_basic());
    suite_add_tcase(suite_records, dns_records_case_runtime());

    sr = srunner_create(suite_records);
    
    srunner_run_all(sr, CK_NORMAL);
    n = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (n == 0) ? 0 : -1;
}
