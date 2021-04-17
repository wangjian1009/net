#include <event2/event.h>
#include "cmocka_all.h"
#include "net_progress.h"
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

    const char * argv[] = { "-la", NULL };
    
    net_progress_t progress =
        net_progress_create(
            net_driver_from_data(env->m_driver), "/bin/ls", argv,
            net_progress_runing_mode_read,
            NULL, NULL, net_progress_debug_text);
    assert_true(progress);

    assert_int_equal(1, event_base_dispatch(env->m_event_base));

    assert_string_equal(
        net_progress_state_str(net_progress_state_complete),
        net_progress_state_str(net_progress_state(progress)));

    assert_int_equal(0, net_progress_exit_state(progress));
    
    assert_true(net_progress_buf_size(progress) > 0);
}

int net_driver_sock_progress_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(driver_sock_progress_basic_test, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
