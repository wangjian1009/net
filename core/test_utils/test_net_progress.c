#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_schedule.h"
#include "test_net_progress.h"
#include "test_net_tl_op.h"

struct test_net_progress_init_setup {
    int m_rv;
    int64_t m_delay_exit;
    int m_exit_stat;
    uint32_t m_output_size;
    void * m_output;
};

struct test_net_progress_complete_op {
    uint32_t m_ep_id;
    struct test_net_progress_init_setup * m_setup;
};

void test_net_progress_complete_op_cb(void * ctx, test_net_tl_op_t op) {
    test_net_driver_t driver = op->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));

    struct test_net_progress_complete_op * complete_op = test_net_tl_op_data(op);
    net_progress_t progress = net_progress_find(schedule, complete_op->m_ep_id);
    if (progress != NULL) {
        switch(net_progress_debug_mode(progress)) {
        case net_progress_debug_none:
            break;
        case net_progress_debug_text:
            CPE_INFO(
                net_schedule_em(schedule), "test: %s: << %d data\n%.*s",
                net_progress_dump(net_schedule_tmp_buffer(schedule), progress),
                complete_op->m_setup->m_output_size,
                complete_op->m_setup->m_output_size, (const char *)complete_op->m_setup->m_output);
            break;
        case net_progress_debug_binary:
            break;
        }
        
        net_progress_buf_append(
            progress,
            complete_op->m_setup->m_output, complete_op->m_setup->m_output_size);
        net_progress_set_complete(progress, complete_op->m_setup->m_exit_stat);
    }
}

int test_net_progress_init(
    net_progress_t base_progress,
    const char * cmd_bin, const char * cmd_argv[], net_progress_runing_mode_t e_mode)
{
    net_driver_t base_driver = net_progress_driver(base_progress);
    test_net_driver_t driver = test_net_driver_cast(base_driver);
    
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

    if (setup->m_delay_exit >= 0) {
        test_net_tl_op_t op =
            test_net_tl_op_create(
                driver, setup->m_delay_exit,
                sizeof(struct test_net_progress_complete_op),
                test_net_progress_complete_op_cb, NULL, NULL);
        assert_true(op != NULL);

        struct test_net_progress_complete_op * op_data = test_net_tl_op_data(op);
        op_data->m_ep_id = net_progress_id(base_progress);
        op_data->m_setup = setup;
    }
    
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
    setup->m_output_size = 0;
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
    setup->m_output_size = 0;
    setup->m_output = NULL;

    test_net_progress_expect_execute_setup_init(driver, ep_id, cmd, e_mode, setup);
}

void test_net_progress_expect_execute_complete(
    test_net_driver_t driver, const char * cmd, net_progress_runing_mode_t mode,
    int exit_rv, const char * output, int64_t delay_ms)
{
    struct test_net_progress_init_setup * setup =
        mem_buffer_alloc(&driver->m_setup_buffer, sizeof(struct test_net_progress_init_setup));
    setup->m_rv = 0;
    setup->m_delay_exit = delay_ms;
    setup->m_exit_stat = exit_rv;
    setup->m_output_size = output ? strlen(output) : 0;
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
