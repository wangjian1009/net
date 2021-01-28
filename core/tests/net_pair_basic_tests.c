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

static void pair_init(void **state) {
    net_pair_testenv_t env = *state;
    
    net_endpoint_t ep[2];
    net_pair_testenv_make_pair(env, ep);

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[0])),
        net_endpoint_state_str(net_endpoint_state_disable));

    assert_string_equal(
        net_endpoint_state_str(net_endpoint_state(ep[1])),
        net_endpoint_state_str(net_endpoint_state_disable));

    assert_true(net_pair_endpoint_other(ep[0]) == ep[1]);
    assert_true(net_pair_endpoint_other(ep[1]) == ep[0]);
}

int net_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(pair_init, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
