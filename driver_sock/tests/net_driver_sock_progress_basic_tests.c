#include "cmocka_all.h"
#include "net_driver_sock_testenv.h"
#include "net_driver_sock_tests.h"

static int setup(void **state) {
    net_driver_sock_testenv_t env = net_driver_sock_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_driver_sock_testenv_t env = *state;
    net_driver_sock_testenv_free(env);
    return 0;
}

static void driver_sock_progress_basic_test(void **state) {
    net_driver_sock_testenv_t env = *state;
}

int net_driver_sock_progress_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(driver_sock_progress_basic_test, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
