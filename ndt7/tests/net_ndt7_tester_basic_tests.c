#include "net_schedule.h"
#include "net_ndt7_testenv.h"
#include "net_ndt7_tester.h"
#include "net_ndt7_tests.h"

static int setup(void **state) {
    net_ndt7_testenv_t env = net_ndt7_testenv_create();
    *state = env;

    test_http_svr_testenv_create_mock_svr(
        env->m_http_svr, "measurementlab", "https://locate.measurementlab.net");

    return 0;
}

static int teardown(void **state) {
    net_ndt7_testenv_t env = *state;
    return 0;
}

static void ndt7_tester_basic(void **state) {
    net_ndt7_testenv_t env = *state;

    net_ndt7_testenv_create_tester(env);
    net_ndt7_tester_t tester = env->m_ndt_tester;

    test_net_dns_expect_query_response(env->m_tdns, "locate.measurementlab.net", "1.1.1.1", 0);
    test_net_endpoint_id_expect_connect_to_acceptor(
        env->m_tdriver, 0, "locate.measurementlab.net:443(1.1.1.1:443)", 0, 0);

    test_http_svr_testenv_expect_response(
        env->m_http_svr,
        "https://locate.measurementlab.net",
        "/v2/nearest/ndt/ndt7",
        200, "OK",
        "{\n"
        "  \"results\": [\n"
        "    {\n"
        "      \"machine\": \"mlab2-tpe01.mlab-oti.measurement-lab.org\",\n"
        "      \"location\": {\n"
        "        \"city\": \"Taipei\",\n"
        "        \"country\": \"TW\"\n"
        "      },\n"
        "      \"urls\": {\n"
        "        \"ws:///ndt/v7/download\": \"ws://host1\",\n"
        "        \"ws:///ndt/v7/upload\": \"ws://host2\",\n"
        "        \"wss:///ndt/v7/download\": \"wss://host3\",\n"
        "        \"wss:///ndt/v7/upload\": \"wss://host4\"\n"
        "      }\n"
        "    },\n"
        "    {\n"
        "      \"machine\": \"mlab2-hkg01.mlab-oti.measurement-lab.org\",\n"
        "      \"location\": {\n"
        "        \"city\": \"Hong Kong\",\n"
        "        \"country\": \"HK\"\n"
        "      },\n"
        "      \"urls\": {\n"
        "        \"ws:///ndt/v7/download\": \"ws://host5\",\n"
        "        \"ws:///ndt/v7/upload\": \"ws://host6\",\n"
        "        \"wss:///ndt/v7/download\": \"wss://host7\",\n"
        "        \"wss:///ndt/v7/upload\": \"wss://host7\"\n"
        "      }\n"
        "    }\n"
        "  ]\n"
        "}\n",
        0);

    /*下载连接设置 */
    test_ws_svr_testenv_create_mock_svr(env->m_ws_svr, "host3", "wss://host3:443");
    test_net_dns_expect_query_response(env->m_tdns, "host3", "1.1.2.1", 0);
    test_net_endpoint_id_expect_connect_to_acceptor(
        env->m_tdriver, 0, "host3:443(1.1.2.1:443)", 0, 0);
    
    assert_true(net_ndt7_tester_start(tester) == 0);

    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        "machine=mlab2-tpe01.mlab-oti.measurement-lab.org, country=TW, city=Taipei\n"
        "    ws-upload: ws://host2\n"
        "    ws-download: ws://host1\n"
        "    wss-upload: wss://host4\n"
        "    wss-download: wss://host3\n"
        "machine=mlab2-hkg01.mlab-oti.measurement-lab.org, country=HK, city=Hong Kong\n"
        "    ws-upload: ws://host6\n"
        "    ws-download: ws://host5\n"
        "    wss-upload: wss://host7\n"
        "    wss-download: wss://host7\n"
        ,
        net_ndt7_testenv_dump_tester(
            net_schedule_tmp_buffer(env->m_schedule), tester));
        
    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_done),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));
}

CPE_BEGIN_TEST_SUIT(net_nd7_tester_basic_tests)
    cmocka_unit_test_setup_teardown(ndt7_tester_basic, setup, teardown),
CPE_END_TEST_SUIT(net_nd7_tester_basic_tests)
