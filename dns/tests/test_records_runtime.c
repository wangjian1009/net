#include "net_dns_entry.h"
#include "test_dns_manager.h"

START_TEST(select_circle_1) {
    with_dns_manager_setup_records(
        "gllto.glpals.com:\n"
        "  - azrltovzs.azureedge.net\n"
        "  - gllto.trafficmanager.net\n"
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

START_TEST(select_circle_2) {
    with_dns_manager_setup_records(
        "account.live.com:\n"
        "  - account.msa.msidentity.com\n"
        "account.msa.msidentity.com:\n"
        "  - account.msa.akadns6.net\n"
        "  - account.msa.trafficmanager.net\n"
        "account.msa.akadns6.net:\n"
        "  - prda.aadg.msidentity.com\n"
        "account.msa.trafficmanager.net:\n"
        "  - prda.aadg.msidentity.com\n"
        "prda.aadg.msidentity.com:\n"
        "  - www.prdtm.aadg.akadns.net\n"
        "www.prdtm.aadg.akadns.net:\n"
        "  - www.prd.aa.aadg.akadns.net\n"
        "www.prd.aa.aadg.akadns.net:\n"
        "  - 40.126.12.34\n"
        "  - 40.126.12.35\n"
        "  - 40.126.12.99\n"
        "  - 40.126.12.101\n"
        "  - 20.190.140.98\n"
        "  - 20.190.140.99\n"
        "  - 20.190.140.100\n"
        "  - 40.126.12.32\n"
        "  - 40.126.12.100\n"
        "  - 40.126.12.33\n"
        );
    
    ck_assert_str_eq(
        with_dns_manager_entry_select_item("account.live.com", net_dns_query_ipv4),
        "40.126.12.34");
}
END_TEST

START_TEST(select_circle_3) {
    with_dns_manager_setup_records(
        "v.lkqd.net");
    
    ck_assert_str_eq(
        with_dns_manager_entry_select_item("v.lkqd.net", net_dns_query_ipv4),
        "40.126.12.34");
}
END_TEST

TCase* dns_records_case_runtime(void) {
    TCase* tc_runtime = tcase_create("runtime");
    tcase_add_checked_fixture(tc_runtime, with_dns_manager_setup, with_dns_manager_teardown);
    tcase_add_test(tc_runtime, select_circle_1);
    tcase_add_test(tc_runtime, select_circle_2);
    return tc_runtime;
}
