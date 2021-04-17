#include "cmocka_all.h"
#include "test_net_progress.h"

struct test_net_progress_init_setup {
    int m_rv;
    int64_t m_delay_exit;
    int m_exit_stat;
    char * m_output;
};

int test_net_progress_init(
    net_progress_t base_progress,
    const char * cmd_bin, const char * cmd_argv[], net_progress_runing_mode_t e_mode)
{
    uint32_t id = net_progress_id(base_progress);
    const char * mode = net_progress_runing_mode_str(e_mode);

    char cmd[1024];
    int n;

    n = snprintf(cmd, sizeof(cmd), "%s", cmd_bin);
    int i;
    for(i = 0; i < 100; ++i) {
        if (cmd_argv[i] == NULL) break;
        n += snprintf(cmd + n, sizeof(cmd) - n, " %s", cmd_argv[i]);
    }
    
    check_expected(id);
    check_expected(cmd);
    check_expected(mode);

    struct test_net_progress_init_setup * setup = mock_type(struct test_net_progress_init_setup *);

    
    return setup->m_rv;
}

void test_net_progress_fini(net_progress_t base_progress) {
    uint32_t id = net_progress_id(base_progress);

    check_expected(id);
    function_called();
}

void test_net_progress_expect_execute_setup_init(
    test_net_driver_t driver, uint32_t ep_id, const char * cmd, net_progress_runing_mode_t e_mode,
    struct test_net_progress_init_setup * setup)
{
    if (ep_id) {
        expect_value(test_net_progress_init, id, ep_id);
    }
    else {
        expect_any(test_net_progress_init, id);
    }

    if (cmd) {
        expect_string(test_net_progress_init, cmd, mem_buffer_strdup(&driver->m_setup_buffer, cmd));
    }
    else {
        expect_any(test_net_progress_init, cmd);
    }

    const char * mode = net_progress_runing_mode_str(e_mode);
    expect_string(test_net_progress_init, mode, mode);

    will_return(test_net_progress_init, setup);
}
    
void test_net_progress_expect_execute_begin_success(
    test_net_driver_t driver, uint32_t ep_id, const char * cmd, net_progress_runing_mode_t e_mode)
{
    struct test_net_progress_init_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_progress_init_setup));
    setup->m_rv = 0;
    setup->m_delay_exit = -1;
    setup->m_exit_stat = -1;
    setup->m_output = NULL;

    test_net_progress_expect_execute_setup_init(driver, ep_id, cmd, e_mode, setup);
}

void test_net_progress_expect_execute_begin_fail(
    test_net_driver_t driver, uint32_t ep_id, const char * cmd, net_progress_runing_mode_t e_mode)
{
    struct test_net_progress_init_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_progress_init_setup));
    setup->m_rv = -1;
    setup->m_delay_exit = -1;
    setup->m_exit_stat = -1;
    setup->m_output = NULL;

    test_net_progress_expect_execute_setup_init(driver, ep_id, cmd, e_mode, setup);
}

void test_net_progress_expect_execute_complete(
    test_net_driver_t driver, const char * cmd, net_progress_runing_mode_t mode,
    int exit_rv, const char * output, int64_t delay_ms)
{
    struct test_net_progress_init_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_progress_init_setup));
    setup->m_rv = -1;
    setup->m_delay_exit = delay_ms;
    setup->m_exit_stat = exit_rv;
    setup->m_output = output ? mem_buffer_strdup(&driver->m_setup_buffer, output) : NULL;

    test_net_progress_expect_execute_setup_init(driver, 0, cmd, mode, setup);
}

void test_net_progress_expect_fini(test_net_driver_t driver, uint32_t ep_id) {
    if (ep_id) {
        expect_value(test_net_progress_fini, id, ep_id);
    }
    else {
        expect_any(test_net_progress_fini, id);
    }

    expect_function_call(test_net_progress_fini);
}
