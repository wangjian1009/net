#include "net_dns_entry.h"
#include "test_dns_manager.h"

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

TCase* dns_records_case_runtime(void) {
    TCase* tc_runtime = tcase_create("runtime");
    tcase_add_checked_fixture(tc_runtime, with_dns_manager_setup, with_dns_manager_teardown);
    tcase_add_test(tc_runtime, select_circle);
    return tc_runtime;
}
