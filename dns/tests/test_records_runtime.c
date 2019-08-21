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
    
    ck_assert_pstr_ne(
        with_dns_manager_entry_select_item("account.live.com", net_dns_query_ipv4),
        NULL);
}
END_TEST

START_TEST(select_circle_3) {
    with_dns_manager_setup_records(
        "s.youtube.com:\n"
        "  - video-stats.l.google.com\n"
        "video-stats.l.google.com:\n"
        "  - 64.233.177.139\n"
        "  - 64.233.177.100\n"
        "  - 64.233.177.138\n"
        "  - 64.233.177.102\n"
        "  - 64.233.177.101\n"
        "  - 173.194.219.101\n"
        "  - 173.194.219.138\n"
        "  - 173.194.219.139\n"
        "  - 173.194.219.100\n"
        "  - 64.233.185.100\n"
        "  - 64.233.185.102\n"
        "  - 64.233.185.113\n"
        "  - 64.233.185.138\n"
        "  - 64.233.185.139\n"
        "  - 64.233.185.101\n"
        "  - 74.125.138.101\n"
        "  - 64.233.188.101\n"
        "  - 64.233.189.102\n"
        "  - 64.233.189.101\n"
        "  - 64.233.189.138\n"
        "  - 64.233.189.100\n"
        "  - 64.233.189.113\n"
        "  - 64.233.189.139\n"
        "  - 108.177.97.102\n"
        "  - 108.177.97.100\n"
        "  - 108.177.97.139\n"
        "  - 108.177.125.100\n"
        "  - 108.177.125.102\n"
        "  - 108.177.125.139\n"
        "  - 108.177.125.101\n"
        "  - 74.125.23.139\n"
        "  - 74.125.23.138\n"
        "  - 108.177.111.100\n"
        "  - 108.177.111.113\n"
        "  - 108.177.111.138\n"
        "  - 108.177.111.139\n"
        "  - 108.177.112.100\n"
        "  - 172.217.214.101\n"
        "  - 172.217.214.102\n"
        "  - 173.194.74.100\n"
        "  - 173.194.192.102\n"
        "  - 173.194.195.139\n"
        "  - 74.125.70.101\n"
        "  - 74.125.126.139\n"
        "  - 74.125.129.113\n"
        "  - 74.125.132.100\n"
        "  - 74.125.132.101\n"
        "  - 74.125.201.138\n"
        "  - 209.85.144.100\n"
        "  - 209.85.144.139\n"
        "  - 172.217.197.139\n"
        "  - 172.217.197.102\n"
        "  - 172.217.197.113\n"
        "  - 172.217.197.138\n"
        "  - 172.217.197.100\n"
        "  - 172.217.197.101\n"
        "  - 173.194.207.113\n"
        "  - 173.194.205.102\n"
        "  - 74.125.192.102\n"
        "  - 209.85.232.100\n"
        "  - 74.125.132.139\n"
        "  - 74.125.201.100\n"
        "  - 74.125.201.113\n"
        "  - 74.125.202.102\n"
        "  - 74.125.69.113\n"
        "  - 74.125.69.138\n"
        "  - 74.125.69.102\n"
        "  - 64.233.181.139\n"
        "  - 64.233.182.138\n"
        "  - 173.194.193.100\n"
        "  - 173.194.196.139\n"
        "  - 173.194.197.100\n"
        "  - 173.194.192.101\n"
        "  - 209.85.234.113\n"
        "  - 108.177.112.139\n"
        "  - 74.125.124.101\n"
        "  - 74.125.124.102\n"
        "  - 108.177.112.113\n"
        "  - 172.217.212.100\n"
        "  - 172.217.212.101\n"
        "  - 172.217.212.102\n"
        "  - 172.217.212.113\n"
        "  - 172.217.212.138\n"
        "  - 172.217.212.139\n"
        "  - 172.217.214.100\n"
        "  - 172.217.214.113\n"
        "  - 172.217.214.138\n"
        "  - 172.217.214.139\n"
        "  - 209.85.234.139\n"
        "  - 74.125.202.100\n"
        "  - 74.125.202.113\n"
        "  - 108.177.112.101\n"
        "  - 108.177.120.101\n"
        "  - 173.194.193.102\n"
        "  - 173.194.197.113\n"
        "  - 64.233.183.101\n"
        "  - 64.233.191.101\n"
        "  - 64.233.191.138\n"
        "  - 74.125.69.100\n"
        "  - 74.125.69.101\n"
        "  - 74.125.124.139\n"
        "  - 74.125.129.138\n"
        "  - 74.125.132.102\n"
        "  - 74.125.201.101\n"
        );
    
    ck_assert_pstr_ne(
        with_dns_manager_entry_select_item("s.youtube.com", net_dns_query_ipv4),
        NULL);
}
END_TEST

TCase* dns_records_case_runtime(void) {
    TCase* tc_runtime = tcase_create("runtime");
    tcase_add_checked_fixture(tc_runtime, with_dns_manager_setup, with_dns_manager_teardown);
    tcase_add_test(tc_runtime, select_circle_1);
    tcase_add_test(tc_runtime, select_circle_2);
    tcase_add_test(tc_runtime, select_circle_3);
    return tc_runtime;
}
