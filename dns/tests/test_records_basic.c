#include "net_dns_entry.h"
#include "test_dns_manager.h"

void test_records_basic_setup(void) {
    with_dns_manager_setup();
    with_dns_manager_setup_records(
        "addr1:\n"
        "  - sub.addr1\n"
        "  - 127.0.0.1\n"
        "sub.addr1: 127.0.0.2\n"
        "sub.addr2: 127.0.0.2\n"
        );
}
    
START_TEST(basic) {
    ck_assert(with_dns_manager_find_entry("addr1") != NULL);
    ck_assert(with_dns_manager_find_entry("sub.addr1") != NULL);
    
    ck_assert_str_eq(with_dns_manager_hostname_by_ip("127.0.0.1"), "addr1");
    ck_assert_str_eq(with_dns_manager_hostname_by_ip("127.0.0.2"), "sub.addr2");
    
    ck_assert_str_eq(with_dns_manager_hostnames_by_ip("127.0.0.1"), "addr1");
    ck_assert_str_eq(with_dns_manager_hostnames_by_ip("127.0.0.2"), "sub.addr2,sub.addr1");
}

START_TEST(add_circle_l1) {
}

END_TEST

TCase* dns_records_case_basic(void) {
    TCase* tc_basic = tcase_create("basic");
    tcase_add_checked_fixture(tc_basic, test_records_basic_setup, with_dns_manager_teardown);
    tcase_add_test(tc_basic, basic);
    tcase_add_test(tc_basic, add_circle_l1);
    return tc_basic;
}
