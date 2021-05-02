#include "net_ndt7_testenv.h"
#include "net_ndt7_tester.h"
#include "net_ndt7_tests.h"

static int setup(void **state) {
    net_ndt7_testenv_t env = net_ndt7_testenv_create();
    *state = env;

    test_http_svr_testenv_create_mock_svr(
        env->m_external_svr, "measurementlab", "https://locate.measurementlab.net");
    
    return 0;
}

static int teardown(void **state) {
    net_ndt7_testenv_t env = *state;
    return 0;
}

static void ndt7_tester_basic(void **state) {
    net_ndt7_testenv_t env = *state;

    net_ndt7_tester_t tester =
        net_ndt7_tester_create(env->m_ndt_manager, net_ndt7_test_download_and_upload);

    test_net_dns_expect_query_response(env->m_tdns, "locate.measurementlab.net", "1.1.1.1", 0);
    test_net_endpoint_id_expect_connect_to_acceptor(
        env->m_tdriver, 0, "locate.measurementlab.net:443(1.1.1.1:443)", 0, 0);

    test_http_svr_testenv_expect_response(
        env->m_external_svr,
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
        "        \"ws:///ndt/v7/download\": \"ws://ndt-mlab2-tpe01.mlab-oti.measurement-lab.org/ndt/v7/download?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItdHBlMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.YdGP7aidlSyWmQy_F9G0xyQXU6xn1PRoguGlTNrgWjaYCeoUG_uJn3xqJxvcZv8P_K8al0QZ0_Tkq34wC7geBg\",\n"
        "        \"ws:///ndt/v7/upload\": \"ws://ndt-mlab2-tpe01.mlab-oti.measurement-lab.org/ndt/v7/upload?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItdHBlMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.YdGP7aidlSyWmQy_F9G0xyQXU6xn1PRoguGlTNrgWjaYCeoUG_uJn3xqJxvcZv8P_K8al0QZ0_Tkq34wC7geBg\",\n"
        "        \"wss:///ndt/v7/download\": \"wss://ndt-mlab2-tpe01.mlab-oti.measurement-lab.org/ndt/v7/download?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItdHBlMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.YdGP7aidlSyWmQy_F9G0xyQXU6xn1PRoguGlTNrgWjaYCeoUG_uJn3xqJxvcZv8P_K8al0QZ0_Tkq34wC7geBg\",\n"
        "        \"wss:///ndt/v7/upload\": \"wss://ndt-mlab2-tpe01.mlab-oti.measurement-lab.org/ndt/v7/upload?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItdHBlMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.YdGP7aidlSyWmQy_F9G0xyQXU6xn1PRoguGlTNrgWjaYCeoUG_uJn3xqJxvcZv8P_K8al0QZ0_Tkq34wC7geBg\"\n"
        "      }\n"
        "    },\n"
        "    {\n"
        "      \"machine\": \"mlab2-hkg01.mlab-oti.measurement-lab.org\",\n"
        "      \"location\": {\n"
        "        \"city\": \"Hong Kong\",\n"
        "        \"country\": \"HK\"\n"
        "      },\n"
        "      \"urls\": {\n"
        "        \"ws:///ndt/v7/download\": \"ws://ndt-mlab2-hkg01.mlab-oti.measurement-lab.org/ndt/v7/download?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItaGtnMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.PtVerWtulK_878Noeaasn1meWzj9qxi_6-vwugedLzr4AIjAQigXMrIq7XyMeZgQY8hxPoBOyxZMNgkEor9SBw\",\n"
        "        \"ws:///ndt/v7/upload\": \"ws://ndt-mlab2-hkg01.mlab-oti.measurement-lab.org/ndt/v7/upload?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItaGtnMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.PtVerWtulK_878Noeaasn1meWzj9qxi_6-vwugedLzr4AIjAQigXMrIq7XyMeZgQY8hxPoBOyxZMNgkEor9SBw\",\n"
        "        \"wss:///ndt/v7/download\": \"wss://ndt-mlab2-hkg01.mlab-oti.measurement-lab.org/ndt/v7/download?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItaGtnMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.PtVerWtulK_878Noeaasn1meWzj9qxi_6-vwugedLzr4AIjAQigXMrIq7XyMeZgQY8hxPoBOyxZMNgkEor9SBw\",\n"
        "        \"wss:///ndt/v7/upload\": \"wss://ndt-mlab2-hkg01.mlab-oti.measurement-lab.org/ndt/v7/upload?access_token=eyJhbGciOiJFZERTQSIsImtpZCI6ImxvY2F0ZV8yMDIwMDQwOSJ9.eyJhdWQiOlsibWxhYjItaGtnMDEubWxhYi1vdGkubWVhc3VyZW1lbnQtbGFiLm9yZyJdLCJleHAiOjE2MTk4Mzg1NjAsImlzcyI6ImxvY2F0ZSIsInN1YiI6Im5kdCJ9.PtVerWtulK_878Noeaasn1meWzj9qxi_6-vwugedLzr4AIjAQigXMrIq7XyMeZgQY8hxPoBOyxZMNgkEor9SBw\"\n"
        "      }\n"
        "    }\n"
        "  ]\n"
        "}\n",
        0);

    assert_true(net_ndt7_tester_start(tester) == 0);

    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        net_ndt7_tester_state_str(net_ndt7_tester_state_done),
        net_ndt7_tester_state_str(net_ndt7_tester_state(tester)));
}

CPE_BEGIN_TEST_SUIT(net_nd7_tester_basic_tests)
    cmocka_unit_test_setup_teardown(ndt7_tester_basic, setup, teardown),
CPE_END_TEST_SUIT(net_nd7_tester_basic_tests)
