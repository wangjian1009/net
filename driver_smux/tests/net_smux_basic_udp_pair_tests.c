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

    net_smux_dgram_t svr_dgram = net_smux_testenv_create_dgram_svr(env, "1.2.3.4:5678");
    net_smux_dgram_t cli_dgram = net_smux_testenv_create_dgram_cli(env, "2.3.4.5:6789");

    net_smux_session_t cli_session =
        net_smux_testenv_dgram_open_session(env, cli_dgram, "1.2.3.4:5678");
    assert_true(cli_session);
    
    net_smux_stream_t cli_stream = net_smux_session_open_stream(cli_session);
}

int net_smux_basic_udp_pair_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_smux_basic_udp_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
