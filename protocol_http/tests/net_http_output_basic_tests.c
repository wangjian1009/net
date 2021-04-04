#include "cmocka_all.h"
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

static void http_req_create_prev_not_complete(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    net_http_req_t req1 = net_http_req_create(ep, net_http_req_method_get, "/a");
    assert_true(req1);

    net_http_req_t req2 = net_http_req_create(ep, net_http_req_method_get, "/b");
    assert_true(req2 == NULL);
}

static void http_req_free_before_head(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    net_http_req_t req1 = net_http_req_create(ep, net_http_req_method_get, "/a");
    assert_true(req1);
    net_http_req_free(req1);

    net_http_req_t req2 = net_http_req_create(ep, net_http_req_method_get, "/b");
    assert_true(req2);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req2);
    assert_true(response != NULL);

    assert_string_equal(
        net_http_testenv_ep_recv_write(env, ep),
        "GET /b HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        );
}

static void http_req_free_part_send_complete(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    net_http_req_t req1 = net_http_req_create(ep, net_http_req_method_get, "/a");
    assert_true(req1);
    assert_true(net_http_req_write_head_pair(req1, "h1", "v1") == 0);
    assert_true(net_http_endpoint_flush(ep) == 0);
    assert_true(net_http_req_write_body_full(req1, NULL, 0) == 0);
    net_http_req_free(req1);

    net_http_req_t req2 = net_http_req_create(ep, net_http_req_method_get, "/b");
    assert_true(req2);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_http_protocol, req2);
    assert_true(response != NULL);

    assert_string_equal(
        net_http_testenv_ep_recv_write(env, ep),
        "GET /a HTTP/1.1\r\n"
        "h1: v1\r\n"
        "Content-Length: 0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        "GET /b HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        );
}

static void http_req_free_part_send_not_complete(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    net_http_req_t req1 = net_http_req_create(ep, net_http_req_method_get, "/a");
    assert_true(req1);
    assert_true(net_http_req_write_head_pair(req1, "h1", "v1") == 0);
    assert_true(net_http_endpoint_flush(ep) == 0);
    net_http_req_free(req1);

    net_http_req_t req2 = net_http_req_create(ep, net_http_req_method_get, "/b");
    assert_true(req2 == NULL);
}

int net_http_output_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(http_req_create_prev_not_complete, setup, teardown),
		cmocka_unit_test_setup_teardown(http_req_free_before_head, setup, teardown),
		cmocka_unit_test_setup_teardown(http_req_free_part_send_complete, setup, teardown),
		cmocka_unit_test_setup_teardown(http_req_free_part_send_not_complete, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
