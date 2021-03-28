#include "cmocka_all.h"
#include "net_http_test_response.h"
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

    net_http_req_t req =
        net_http_svr_testenv_create_req(
            env->m_http_svr_env, net_http_req_method_get, "prometheus");

    net_http_test_response_t response = 
        net_http_svr_testenv_req_commit(env->m_http_svr_env, req);

    test_net_driver_run(env->m_net_driver, 0);

    assert_string_equal(
        "aa",
        net_http_test_response_body_to_string(response));
}

int prometheus_http_basic_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_get, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
