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
    net_http_endpoint_t ep = net_https_testenv_create_ep(env);

    net_http_req_t req = net_http_req_create(ep, net_http_req_method_get, "/a");
    assert_true(req);

    net_http_test_response_t response = net_http_test_protocol_req_commit(env->m_env->m_http_protocol, req);
    assert_true(response != NULL);

    assert_true(
        net_http_testenv_send_response(
            env->m_env, ep,
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
        net_http_test_response_dump(&env->m_env->m_tmp_buffer, response));
}

int net_https_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(https_input_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
