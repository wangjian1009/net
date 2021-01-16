#include "cmocka_all.h"
#include "net_endpoint.h"
#include "net_ssl_tests.h"
#include "net_ssl_pair_testenv.h"

static int setup(void **state) {
    net_ssl_pair_testenv_t env = net_ssl_pair_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ssl_pair_testenv_t env = *state;
    net_ssl_pair_testenv_free(env);
    return 0;
}

static void net_ssl_pair_basic(void **state) {
    net_ssl_pair_testenv_t env = *state;
    net_endpoint_t cli_ep = net_ssl_testenv_create_cli_ep(env->m_env);
    net_endpoint_set_remote_address(cli_ep, net_acceptor_address(env->m_acceptor));
    net_endpoint_connect(cli_ep);
}

int net_ssl_pair_basic_tests() {
	const struct CMUnitTest ssl_basic_tests[] = {
		cmocka_unit_test_setup_teardown(net_ssl_pair_basic, setup, teardown),
	};
	return cmocka_run_group_tests(ssl_basic_tests, NULL, NULL);
}
