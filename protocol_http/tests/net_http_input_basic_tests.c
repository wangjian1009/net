#include "cmocka_all.h"
#include "net_endpoint.h"
#include "net_http_testenv.h"
#include "net_http_test_response.h"
#include "net_http_tests.h"
#include "net_http_req.h"
#include "net_http_endpoint.h"

static int setup(void **state) {
    net_http_testenv_t env = net_http_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_http_testenv_t env = *state;
    net_http_testenv_free(env);
    return 0;
}

static void http_input_identity(void **state) {
    net_http_testenv_t env = *state;
    net_http_testenv_create_ep_established(env);

    net_http_req_t req = net_http_req_create(env->m_http_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);

    assert_true(
        net_http_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Connection: Keep-alive\r\n"
            "\r\n"
            ) == 0
        );

    assert_string_equal(
        "200 OK\n"
        "Connection=Keep-alive\n"
        "body.size=0\n"
        ,
        net_http_test_response_dump(&env->m_tmp_buffer, response));

    assert_true(net_http_req_find(env->m_http_endpoint, response->m_req_id) == NULL);
}

static void http_input_chunked(void **state) {
    net_http_testenv_t env = *state;
    net_http_testenv_create_ep_established(env);

    net_http_req_t req = net_http_req_create(env->m_http_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);

    assert_true(
        net_http_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "7\r\n"
            "Mozilla\r\n"
            "9\r\n"
            "Developer\r\n"
            "7\r\n"
            "Network\r\n"
            "0\r\n"
            "\r\n"
            ) == 0
        );

    assert_string_equal(
        "200 OK\n"
        "Content-Type=text/plain\n"
        "Transfer-Encoding=chunked\n"
        "body.size=23\n"
        "MozillaDeveloperNetwork"
        ,
        net_http_test_response_dump(&env->m_tmp_buffer, response));

    assert_true(net_http_req_find(env->m_http_endpoint, response->m_req_id) == NULL);
}

static void http_input_gzip_identity(void **state) {
    net_http_testenv_t env = *state;
    net_http_testenv_create_ep_established(env);

    net_http_req_t req = net_http_req_create(env->m_http_endpoint, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);
    assert_true(response != NULL);

    const char * gzip_hex_date = 
        "1f8b08088269a2600003612e74787400"
        "4b4c4a4e494d4bcfc8cccacec9cde302"
        "0008f2496c0f000000";

    assert_true(
        net_http_testenv_send_response(
            env,
            "HTTP/1.1 200 OK\r\n"
            "Content-Encoding: gzip\r\n"
            "Content-Length: 41\r\n"
            "\r\n"
            ) == 0
        );

    assert_true(net_http_testenv_send_response_hex(env, gzip_hex_date) == 0);
    
    assert_string_equal(
        "200 OK\n"
        "Content-Encoding=gzip\n"
        "body.size=14\n"
        "abcdefghijklmn",
        net_http_test_response_dump(&env->m_tmp_buffer, response));

    assert_true(net_http_req_find(env->m_http_endpoint, response->m_req_id) == NULL);
}

CPE_BEGIN_TEST_SUIT(net_http_input_basic_tests)
    cmocka_unit_test_setup_teardown(http_input_identity, setup, teardown),
    cmocka_unit_test_setup_teardown(http_input_chunked, setup, teardown),
    cmocka_unit_test_setup_teardown(http_input_gzip_identity, setup, teardown),
CPE_END_TEST_SUIT(net_http_input_basic_tests)
