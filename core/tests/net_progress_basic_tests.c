#include "cmocka_all.h"
#include "test_net_progress.h"
#include "net_progress.h"
#include "net_progress_testenv.h"

static int setup(void **state) {
    net_progress_testenv_t env = net_progress_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_progress_testenv_t env = *state;
    net_progress_testenv_free(env);
    return 0;
}

static void net_progress_basic(void **state) {
    net_progress_testenv_t env = *state;

    test_net_progress_expect_execute_begin_success(
        env->m_env->m_tdriver, 0, "cmd1", net_progress_runing_mode_read);
    
    net_progress_t progress =
        net_progress_testenv_run_cmd_read(env, "cmd1");

    assert_string_equal(
        net_progress_state_str(net_progress_state_runing),
        net_progress_state_str(net_progress_state(progress)));

    assert_true(net_progress_buf_append(progress, "abcd", 4) == 0);
    assert_true(net_progress_buf_append(progress, "de", 2) == 0);

    assert_string_equal(
        "abcdde",
        net_progress_testenv_buffer_to_string(env));

    assert_true(net_progress_complete(progress) == 0);
    assert_string_equal(
        net_progress_state_str(net_progress_state_complete),
        net_progress_state_str(net_progress_state(progress)));
    
    test_net_progress_expect_fini(env->m_env->m_tdriver, 0);
    net_progress_free(progress);
}

int net_progress_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(net_progress_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
