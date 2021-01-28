#include "cmocka_all.h"
#include "net_http_testenv.h"
#include "net_http_testenv_response.h"
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

static void http_req_get_no_body(void **state) {
    net_http_testenv_t env = *state;

    net_http_endpoint_t ep = net_http_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(ep, net_http_req_method_get, "/a/b/c");

    net_http_testenv_response_t response = net_http_testenv_req_commit(env, req);

    assert_true(response != NULL);

    assert_string_equal(
        net_http_testenv_ep_recv_write(env, ep),
        "GET /a/b/c HTTP/1.1\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        );
}

int net_req_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(http_req_get_no_body, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
