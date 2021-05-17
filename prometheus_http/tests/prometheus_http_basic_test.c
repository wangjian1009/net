#include "test_net_endpoint.h"
#include "net_http_test_response.h"
#include "net_http_endpoint.h"
#include "prometheus_metric.h"
#include "prometheus_counter.h"
#include "prometheus_collector.h"
#include "prometheus_http_tests.h"
#include "prometheus_http_testenv.h"

static int setup(void **state) {
    prometheus_http_testenv_t env = prometheus_http_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_http_testenv_t env = *state;
    prometheus_http_testenv_free(env);
    return 0;
}

static void test_get(void **state) {
    prometheus_http_testenv_t env = *state;

    prometheus_counter_t counter = 
        prometheus_counter_create(env->m_manager, "TestName", "TestHelp", 0, NULL);

    prometheus_collector_add_metric(
        prometheus_collector_default(env->m_manager),
        prometheus_metric_from_data(counter));

    prometheus_counter_add(counter, 1234.0, NULL);
    
    net_http_req_t req = prometheus_http_testenv_create_req(env, net_http_req_method_get, "metric");

    net_http_test_response_t response = 
        test_http_svr_testenv_req_commit(env->m_http_svr_env, req);

    test_net_driver_run(env->m_net_driver, 0);

    assert_int_equal(200,  response->m_code);
    
    assert_string_equal(
        "# HELP TestName TestHelp\n"
        "# TYPE TestName counter\n"
        "TestName 1234\n",
        net_http_test_response_body_to_string(response));
}

static void test_method_error(void **state) {
    prometheus_http_testenv_t env = *state;

    net_http_req_t req = prometheus_http_testenv_create_req(env, net_http_req_method_post, "metric");

    net_http_test_response_t response = 
        test_http_svr_testenv_req_commit(env->m_http_svr_env, req);

    test_net_driver_run(env->m_net_driver, 0);

    assert_int_equal(400,  response->m_code);
    assert_string_equal("Invalid HTTP Method", response->m_code_msg);
    assert_string_equal("", net_http_test_response_body_to_string(response));
}

static void test_path_error(void **state) {
    prometheus_http_testenv_t env = *state;

    net_http_req_t req = prometheus_http_testenv_create_req(env, net_http_req_method_get, "metric/aa");

    net_http_test_response_t response = 
        test_http_svr_testenv_req_commit(env->m_http_svr_env, req);

    test_net_driver_run(env->m_net_driver, 0);

    assert_int_equal(400,  response->m_code);
    assert_string_equal("Bad Request", response->m_code_msg);
    assert_string_equal("", net_http_test_response_body_to_string(response));
}

static void test_response_write_error(void **state) {
    prometheus_http_testenv_t env = *state;

    prometheus_counter_t counter = 
        prometheus_counter_create(env->m_manager, "TestName", "TestHelp", 0, NULL);

    prometheus_collector_add_metric(
        prometheus_collector_default(env->m_manager),
        prometheus_metric_from_data(counter));

    prometheus_counter_add(counter, 1234.0, NULL);
    
    net_http_req_t req = prometheus_http_testenv_create_req(env, net_http_req_method_get, "metric");
    assert_true(req != NULL);

    net_endpoint_t svr_ep = test_http_svr_testenv_req_svr_endpoint(env->m_http_svr_env, req);
    assert_true(svr_ep != NULL);
    test_net_endpoint_expect_write_error(
        svr_ep,
        net_endpoint_error_source_network, net_endpoint_network_errno_internal, "write error", 0);
    
    net_http_test_response_t response = 
        test_http_svr_testenv_req_commit(env->m_http_svr_env, req);

    test_net_driver_run(env->m_net_driver, 0);
}

CPE_BEGIN_TEST_SUIT(prometheus_http_basic_tests)
    cmocka_unit_test_setup_teardown(test_get, setup, teardown),
    cmocka_unit_test_setup_teardown(test_method_error, setup, teardown),
    cmocka_unit_test_setup_teardown(test_path_error, setup, teardown),
    cmocka_unit_test_setup_teardown(test_response_write_error, setup, teardown),
CPE_END_TEST_SUIT(prometheus_http_basic_tests)
