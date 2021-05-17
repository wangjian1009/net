#include "cmocka_all.h"
#include "net_http_tests.h"
#include "net_http_test_response.h"
#include "net_http_tests.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"
#include "net_https_testenv.h"

static int setup(void **state) {
    net_https_testenv_t env = net_https_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_https_testenv_t env = *state;
    net_https_testenv_free(env);
    return 0;
}

static void https_input_basic(void **state) {
    net_https_testenv_t env = *state;
    net_https_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(env->m_https_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);
    test_net_driver_run(env->m_tdriver, 0);

    assert_true(
        net_https_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Connection: Keep-alive\r\n"
            ) == 0
        );
    test_net_driver_run(env->m_tdriver, 0);

    assert_string_equal(
        "200 OK\n"
        "Connection=Keep-alive\n"
        "body.size=0\n"
        ,
        net_http_test_response_dump(&env->m_tmp_buffer, response));
}

static void https_input_part_read_close(void **state) {
    net_https_testenv_t env = *state;
    net_https_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(env->m_https_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);
    test_net_driver_run(env->m_tdriver, 0);

    assert_true(
        net_https_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Connection: Keep-alive\r\n"
            ) == 0
        );
    test_net_driver_run(env->m_tdriver, 0);
    assert_true(net_http_req_find(env->m_https_endpoint, response->m_req_id) != NULL);

    assert_true(net_https_testenv_close_read(env) == 0);
    assert_true(net_http_req_find(env->m_https_endpoint, response->m_req_id) == NULL);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state_disable),
        net_endpoint_state_str(net_endpoint_state(net_http_endpoint_base_endpoint(env->m_https_endpoint))));

    assert_string_equal(
        net_http_res_result_str(net_http_res_conn_disconnected),
        net_http_res_result_str(response->m_result));
    
    assert_string_equal(
        "200 OK\n"
        "Connection=Keep-alive\n"
        "body.size=0\n"
        ,
        net_http_test_response_dump(&env->m_tmp_buffer, response));
}

static void https_input_part_write_close(void **state) {
    net_https_testenv_t env = *state;
    net_https_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(env->m_https_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);
    test_net_driver_run(env->m_tdriver, 0);

    assert_true(
        net_https_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Connection: Keep-alive\r\n"
            ) == 0
        );
    test_net_driver_run(env->m_tdriver, 0);
    assert_true(net_http_req_find(env->m_https_endpoint, response->m_req_id) != NULL);

    /*请求过程中，写入关闭，请求依旧还能等待数据 */
    assert_true(net_https_testenv_close_write(env) == 0);
    assert_true(net_http_req_find(env->m_https_endpoint, response->m_req_id) != NULL);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state_write_closed),
        net_endpoint_state_str(net_endpoint_state(net_http_endpoint_base_endpoint(env->m_https_endpoint))));

    /*write-close 状态下不能创建新请求 */
    assert_true(net_http_req_create(env->m_https_endpoint, net_http_req_method_get, "/b") == NULL);

    /*write-close 状态下还可以将请求接受完成 */
    assert_true(net_https_testenv_send_response(env, "\r\n") == 0);
    test_net_driver_run(env->m_tdriver, 0);
    assert_true(net_http_req_find(env->m_https_endpoint, response->m_req_id) == NULL);

    assert_string_equal(
        net_http_res_result_str(net_http_res_complete),
        net_http_res_result_str(response->m_result));
    
    assert_string_equal(
        "200 OK\n"
        "Connection=Keep-alive\n"
        "body.size=0\n"
        ,
        net_http_test_response_dump(&env->m_tmp_buffer, response));

    /*接受完成以后连接应该被关闭 */
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state_disable),
        net_endpoint_state_str(net_endpoint_state(net_http_endpoint_base_endpoint(env->m_https_endpoint))));
}

CPE_BEGIN_TEST_SUIT(net_https_basic_tests)
    cmocka_unit_test_setup_teardown(https_input_basic, setup, teardown),
    cmocka_unit_test_setup_teardown(https_input_part_read_close, setup, teardown),
    cmocka_unit_test_setup_teardown(https_input_part_write_close, setup, teardown),
CPE_END_TEST_SUIT(net_https_basic_tests)
