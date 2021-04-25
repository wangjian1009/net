#include "cmocka_all.h"
#include "net_http_testenv.h"
#include "net_http_test_response.h"
#include "net_http_tests.h"
#include "net_http_req.h"

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

static void http_method_head_response_no_content_lenth(void **state) {
    net_http_testenv_t env = *state;

    net_http_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(env->m_http_endpoint, net_http_req_method_head, "/a/b/c");

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);

    assert_true(response != NULL);

    assert_string_equal(
        net_http_testenv_ep_recv_write(env),
        "HEAD /a/b/c HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        );

    assert_true(
        net_http_testenv_ep_send_response(
            env,
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://157.240.211.35/\r\n"
            "Content-Type: text/html; charset=\"utf-8\"\r\n"
            "Content-Length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n")
        == 0);
}

static void http_method_head_response_with_content_lenth(void **state) {
    net_http_testenv_t env = *state;

    net_http_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(env->m_http_endpoint, net_http_req_method_head, "/a/b/c");

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req);

    assert_true(response != NULL);

    assert_string_equal(
        net_http_testenv_ep_recv_write(env),
        "HEAD /a/b/c HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        );

    assert_true(
        net_http_testenv_ep_send_response(
            env,
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Location: https://157.240.211.35/\r\n"
            "Content-Type: text/html; charset=\"utf-8\"\r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: 0\r\n"
            "\r\n")
        == 0);
}

int net_http_output_method_head_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(http_method_head_response_with_content_lenth, setup, teardown),
		cmocka_unit_test_setup_teardown(http_method_head_response_no_content_lenth, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
