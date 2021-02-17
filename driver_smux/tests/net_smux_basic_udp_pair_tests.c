#include "cmocka_all.h"
#include "net_smux_tests.h"
#include "net_smux_testenv.h"

static int setup(void **state) {
    net_smux_testenv_t env = net_smux_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_smux_testenv_t env = *state;
    net_smux_testenv_free(env);
    return 0;
}

static void net_smux_basic_udp_pair_basic(void **state) {
    net_smux_testenv_t env = *state;

    net_smux_session_t svr_session = net_smux_testenv_create_session_udp_svr(env, "1.2.3.4:5678");
    net_smux_session_t cli_session = net_smux_testenv_create_session_udp_cli(env, "1.2.3.4:5678");

}

int net_smux_basic_udp_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_smux_basic_udp_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
