#include "net_dns_entry.h"
#include "test_dns_manager.h"

START_TEST(basic) {
    with_dns_manager_add_records(
        "addr1:\n"
        "  - sub.addr1\n"
        "  - 127.0.0.1\n"
        "sub.addr1: 127.0.0.2\n"
        "sub.addr2: 127.0.0.2\n"
        );
    
    ck_assert(with_dns_manager_find_entry("addr1") != NULL);
    ck_assert(with_dns_manager_find_entry("sub.addr1") != NULL);
    
    ck_assert_str_eq(with_dns_manager_hostname_by_ip("127.0.0.1"), "addr1");
    ck_assert_str_eq(with_dns_manager_hostname_by_ip("127.0.0.2"), "sub.addr2");
    
    ck_assert_str_eq(with_dns_manager_hostnames_by_ip("127.0.0.1"), "addr1");
    ck_assert_str_eq(with_dns_manager_hostnames_by_ip("127.0.0.2"), "sub.addr2,sub.addr1");
}
END_TEST

START_TEST(select_circle) {
    with_dns_manager_add_records(
        "gllto.glpals.com:\n"
        "  - azrltovzs.azureedge.net\n"
        "  - d1iskralo6mo11.cloudfront.net\n"
        "  - azrltovzs.azureedge.net\n"
        );
    
    net_dns_entry_t entry = with_dns_manager_find_entry("gllto.glpals.com");
    ck_assert(entry != NULL);
    
    net_dns_entry_item_t item = net_dns_entry_select_item(entry, net_dns_item_select_policy_first, net_dns_query_ipv4);
    ck_assert(item);
}
END_TEST

Suite* dns_records_suite(void) {
    Suite* s = suite_create("dns_records");

    TCase* tc_basic = tcase_create("basic");
    tcase_add_checked_fixture(tc_basic, with_dns_manager_setup, with_dns_manager_teardown);
    suite_add_tcase(s, tc_basic);
    tcase_add_test(tc_basic, basic);
    
    TCase* tc_runtime = tcase_create("runtime");
    tcase_add_checked_fixture(tc_runtime, with_dns_manager_setup, with_dns_manager_teardown);
    suite_add_tcase(s, tc_runtime);
    tcase_add_test(tc_runtime, select_circle);
        
    return s;
}
