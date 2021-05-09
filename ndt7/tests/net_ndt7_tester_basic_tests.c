#include "net_schedule.h"
#include "net_ndt7_testenv.h"
#include "net_ndt7_config.h"
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

    struct net_ndt7_config cfg = *net_ndt7_tester_config(tester);
    cfg.m_measurement_interval_ms = 250;
    net_ndt7_tester_set_config(tester, &cfg);

    test_ws_svr_testenv_create_mock_svr(env->m_ws_svr, "host4", "wss://host4:443");
    
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

    /*测试查询节点 */
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

    /*测试下载过程 */
    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_download),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));

    net_ndt7_testenv_download_send_bin(env, 5, 250);
    test_net_driver_run(env->m_tdriver, 250);
    assert_string_equal(
        "{\"AppInfo\":{\"ElapsedTime\":250,\"NumBytes\":5},\"Origin\":\"Client\",\"Test\":\"download\"}",
        net_ndt7_response_dump(net_schedule_tmp_buffer(env->m_schedule), &env->m_last_response));

    net_ndt7_testenv_download_send_text(
        env,
        "{\"ConnectionInfo\":{\"Client\":\"69.172.67.64:58982\",\"Server\":\"183.178.65.24:443\",\"UUID\":\"ndt-tcnnn_1619905339_0000000000040335\"}"
        ",\"BBRInfo\":{\"BW\":108295,\"MinRTT\":45825,\"PacingGain\":739,\"CwndGain\":739,\"ElapsedTime\":590111}"
        ",\"TCPInfo\":{\"State\":1,\"CAState\":0,\"Retransmits\":0,\"Probes\":0,\"Backoff\":0,\"Options\":7,\"WScale\":118,\"AppLimited\":0,\"RTO\":588000,\"ATO\":40000,\"SndMSS\":1388,\"RcvMSS\":564,\"Unacked\":18,\"Sacked\":0,\"Lost\":0,\"Retrans\":0,\"Fackets\":0,\"LastDataSent\":44,\"LastAckSent\":0,\"LastDataRecv\":592,\"LastAckRecv\":44,\"PMTU\":1500,\"RcvSsThresh\":64076,\"RTT\":227081,\"RTTVar\":83149,\"SndSsThresh\":2147483647,\"SndCwnd\":19,\"AdvMSS\":1448,\"Reordering\":3,\"RcvRTT\":0,\"RcvSpace\":14600,\"TotalRetrans\":0,\"PacingRate\":557746,\"MaxPacingRate\":-1,\"BytesAcked\":43900,\"BytesReceived\":1093,\"SegsOut\":57,\"SegsIn\":34,\"NotsentBytes\":105488,\"MinRTT\":45825,\"DataSegsIn\":4,\"DataSegsOut\":53,\"DeliveryRate\":80680,\"BusyTime\":808000,\"RWndLimited\":0,\"SndBufLimited\":0,\"Delivered\":36,\"DeliveredCE\":0,\"BytesSent\":68884,\"BytesRetrans\":0,\"DSackDups\":0,\"ReordSeen\":0,\"RcvOooPack\":0,\"SndWnd\":129664,\"ElapsedTime\":590111}"
        "}",
        0);
    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        "{\"ConnectionInfo\":{\"Client\":\"69.172.67.64:58982\",\"Server\":\"183.178.65.24:443\",\"UUID\":\"ndt-tcnnn_1619905339_0000000000040335\"}"
        ",\"BBRInfo\":{\"BW\":108295,\"MinRTT\":45825,\"PacingGain\":739,\"CwndGain\":739,\"ElapsedTime\":590111}"
        ",\"TCPInfo\":{\"State\":1,\"CAState\":0,\"Retransmits\":0,\"Probes\":0,\"Backoff\":0,\"Options\":7,\"WScale\":118,\"AppLimited\":0,\"RTO\":588000,\"ATO\":40000,\"SndMSS\":1388,\"RcvMSS\":564,\"Unacked\":18,\"Sacked\":0,\"Lost\":0,\"Retrans\":0,\"Fackets\":0,\"LastDataSent\":44,\"LastAckSent\":0,\"LastDataRecv\":592,\"LastAckRecv\":44,\"PMTU\":1500,\"RcvSsThresh\":64076,\"RTT\":227081,\"RTTVar\":83149,\"SndSsThresh\":2147483647,\"SndCwnd\":19,\"AdvMSS\":1448,\"Reordering\":3,\"RcvRTT\":0,\"RcvSpace\":14600,\"TotalRetrans\":0,\"PacingRate\":557746,\"MaxPacingRate\":-1,\"BytesAcked\":43900,\"BytesReceived\":1093,\"SegsOut\":57,\"SegsIn\":34,\"NotsentBytes\":105488,\"MinRTT\":45825,\"DataSegsIn\":4,\"DataSegsOut\":53,\"DeliveryRate\":80680,\"BusyTime\":808000,\"RWndLimited\":0,\"SndBufLimited\":0,\"Delivered\":36,\"DeliveredCE\":0,\"BytesSent\":68884,\"BytesRetrans\":0,\"DSackDups\":0,\"ReordSeen\":0,\"RcvOooPack\":0,\"SndWnd\":129664,\"ElapsedTime\":590111}"
        "}",
        net_ndt7_measurement_dump(net_schedule_tmp_buffer(env->m_schedule), &env->m_last_measurement));

    net_ndt7_testenv_download_close(env, net_ws_status_code_normal_closure, NULL, 0);

    /*设置上传数据 */
    test_net_dns_expect_query_response(env->m_tdns, "host4", "1.1.3.1", 0);
    test_net_endpoint_id_expect_connect_to_acceptor(
        env->m_tdriver, 0, "host4:443(1.1.3.1:443)", 0, 0);
    
    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_upload),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));
    
    /*测试上载过程 */

    /**/
    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_done),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));
}

CPE_BEGIN_TEST_SUIT(net_nd7_tester_basic_tests)
    cmocka_unit_test_setup_teardown(ndt7_tester_basic, setup, teardown),
CPE_END_TEST_SUIT(net_nd7_tester_basic_tests)
