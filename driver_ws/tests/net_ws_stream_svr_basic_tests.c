#include "cmocka_all.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"
#include "test_net_endpoint.h"

static int setup(void **state) {
    net_ws_testenv_t env = net_ws_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_testenv_free(env);
    return 0;
}

static void net_ws_stream_svr_underline_established(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_stream_endpoint_t endpoint = net_ws_testenv_create_stream(env);
    net_endpoint_t base_endpoint = net_ws_stream_endpoint_base_endpoint(endpoint);
    net_endpoint_t underline = net_ws_stream_endpoint_underline(base_endpoint);
    assert_true(underline != NULL);
    
    assert_true(net_endpoint_set_state(underline, net_endpoint_state_established) ==0);
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(base_endpoint)),
        net_endpoint_state_str(net_endpoint_state_connecting));
}

CPE_BEGIN_TEST_SUIT(net_ws_stream_svr_basic_tests)
    cmocka_unit_test_setup_teardown(net_ws_stream_svr_underline_established, setup, teardown),
CPE_END_TEST_SUIT(net_ws_stream_svr_basic_tests)
