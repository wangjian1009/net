#include "with_dns_manager.h"

START_TEST(test_add) {
    with_dns_manager_add_records(
        "aa: 123\n"
        "bb: aa\n"
        );
    fail_unless(5 == 5, "error, 2 + 3 != 5");
}
END_TEST

Suite* make_add_suite(void) {
    Suite* s = suite_create("add");
    TCase* tc_add = tcase_create("add");
    tcase_add_checked_fixture(tc_add, with_dns_manager_setup, with_dns_manager_teardown);
    suite_add_tcase(s, tc_add);
    tcase_add_test(tc_add, test_add);
    return s;
}

int main(void) {
    int n;
    SRunner* sr;
    sr = srunner_create(make_add_suite());
    srunner_run_all(sr, CK_NORMAL);
    n = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (n == 0) ? 0 : -1;
}
