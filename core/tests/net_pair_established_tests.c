#include "cmocka_all.h"
#include "net_pair_testenv.h"
#include "net_core_tests.h"

static int setup(void **state) {
    net_pair_testenv_t env = net_pair_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_pair_testenv_t env = *state;
    net_pair_testenv_free(env);
    return 0;
}

static void pair_established_write_close_ok(void **state) {
    net_pair_testenv_t env = *state;

    net_endpoint_t ep[2];
    net_pair_testenv_make_pair_established(env, ep);

    net_endpoint_set_state(ep[0], net_endpoint_state_write_closed);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[0])),
        net_endpoint_state_str(net_endpoint_state_write_closed));
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[1])),
        net_endpoint_state_str(net_endpoint_state_read_closed));
}

static void pair_established_read_close_ok(void **state) {
    net_pair_testenv_t env = *state;

    net_endpoint_t ep[2];
    net_pair_testenv_make_pair_established(env, ep);

    net_endpoint_set_state(ep[0], net_endpoint_state_read_closed);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[0])),
        net_endpoint_state_str(net_endpoint_state_read_closed));
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[1])),
        net_endpoint_state_str(net_endpoint_state_write_closed));
}

static void pair_established_disable_ok(void **state) {
    net_pair_testenv_t env = *state;

    net_endpoint_t ep[2];
    net_pair_testenv_make_pair_established(env, ep);

    net_endpoint_set_state(ep[0], net_endpoint_state_disable);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[0])),
        net_endpoint_state_str(net_endpoint_state_disable));
    
    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[1])),
        net_endpoint_state_str(net_endpoint_state_disable));
}

int net_pair_established_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(pair_established_write_close_ok, setup, teardown),

		cmocka_unit_test_setup_teardown(pair_established_read_close_ok, setup, teardown),

        cmocka_unit_test_setup_teardown(pair_established_disable_ok, setup, teardown),
    };
	return cmocka_run_group_tests(tests, NULL, NULL);
}
