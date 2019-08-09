#include "net_dns_entry.h"
#include "test_dns_manager.h"

START_TEST(select_circle) {
    with_dns_manager_setup_records(
        "gllto.glpals.com:\n"
        "  - azrltovzs.azureedge.net\n"
        "  - d1iskralo6mo11.cloudfront.net\n"
        "azrltovzs.azureedge.net:\n"
        "  - azrltovzs.ec.azureedge.net\n"
        "azrltovzs.ec.azureedge.net:\n"
        "  - scdn2.wpc.4df59.chicdn.net\n"
        "scdn2.wpc.4df59.chicdn.net:\n"
        "  - sa4gl.wpc.chicdn.net\n"
        "sa4gl.wpc.chicdn.net:\n"
        "  - 152.195.12.4\n"
        "d1iskralo6mo11.cloudfront.net:\n"
        "  - 13.225.212.32\n"
        "  - 13.225.212.60\n"
        "  - 13.225.212.74\n"
        "  - 13.225.212.80\n"
        "gllto.trafficmanager.net:\n"
        "  - azrltovzs.azureedge.net\n"
        );
    
    ck_assert_str_eq(
        with_dns_manager_entry_select_item("gllto.glpals.com", net_dns_query_ipv4),
        "152.195.12.4");
}
END_TEST

TCase* dns_records_case_runtime(void) {
    TCase* tc_runtime = tcase_create("runtime");
    tcase_add_checked_fixture(tc_runtime, with_dns_manager_setup, with_dns_manager_teardown);
    tcase_add_test(tc_runtime, select_circle);
    return tc_runtime;
}
