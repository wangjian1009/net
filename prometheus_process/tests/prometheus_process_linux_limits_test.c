#include "cmocka_all.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/vfs/vfs_file.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_linux_limits_testenv.h"

static int setup(void **state) {
    prometheus_process_linux_limits_testenv_t env = prometheus_process_linux_limits_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;
    prometheus_process_linux_limits_testenv_free(env);
    return 0;
}

static void file_parsing(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    prometheus_process_install_limits(
        env->m_env,
        "Limit                     Soft Limit           Hard Limit           Units\n"
        "Max cpu time              unlimited            unlimited            seconds\n"
        "Max file size             unlimited            unlimited            bytes\n"
        "Max data size             unlimited            unlimited            bytes\n"
        "Max stack size            8388608              unlimited            bytes\n"
        "Max core file size        0                    unlimited            bytes\n"
        "Max resident set          unlimited            unlimited            bytes\n"
        "Max processes             unlimited            unlimited            processes\n"
        "Max open files            1048576              1048576              files\n"
        "Max locked memory         83968000             83968000             bytes\n"
        "Max address space         unlimited            unlimited            bytes\n"
        "Max file locks            unlimited            unlimited            locks\n"
        "Max pending signals       23701                23701                signals\n"
        "Max msgqueue size         819200               819200               bytes\n"
        "Max nice priority         0                    0\n"
        "Max realtime priority     0                    0\n"
        "Max realtime timeout      unlimited            unlimited            us\n"
        );

    assert_true(prometheus_process_linux_limits_testenv_load(env) == 0);

    assert_int_equal(16, env->m_row_count);

    prometheus_process_linux_limits_row_t row =
        prometheus_process_linux_limits_testenv_find_row(env, "Max cpu time");
    assert_true(row != NULL);
    assert_int_equal(-1, row->soft);
    assert_int_equal(-1, row->hard);
    assert_string_equal("Max cpu time", row->limit);
    assert_string_equal("seconds", row->units);

    row = prometheus_process_linux_limits_testenv_find_row(env, "Max file size");
    assert_int_equal(-1, row->soft);
    assert_int_equal(-1, row->hard);
    assert_string_equal("Max file size", row->limit);
    assert_string_equal("bytes", row->units);

    row = prometheus_process_linux_limits_testenv_find_row(env, "Max data size");
    assert_int_equal(-1, row->soft);
    assert_int_equal(-1, row->hard);
    assert_string_equal("Max data size", row->limit);
    assert_string_equal("bytes", row->units);

    row = prometheus_process_linux_limits_testenv_find_row(env, "Max stack size");
    assert_int_equal(8388608, row->soft);
    assert_int_equal(-1, row->hard);
    assert_string_equal("Max stack size", row->limit);
    assert_string_equal("bytes", row->units);

    row = prometheus_process_linux_limits_testenv_find_row(env, "Max processes");
    assert_int_equal(-1, row->soft);
    assert_int_equal(-1, row->hard);
    assert_string_equal("Max processes", row->limit);
    assert_string_equal("processes", row->units);

    row = prometheus_process_linux_limits_testenv_find_row(env, "Max pending signals");
    assert_int_equal(23701, row->soft);
    assert_int_equal(23701, row->hard);
    assert_string_equal("Max pending signals", row->limit);
    assert_string_equal("signals", row->units);
}

static void rdp_next_token(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, " \t!", 4, 0, NULL, NULL};
    assert_true(prometheus_process_linux_limits_rdp_next_token(&ctx) == 0);

    assert_int_equal(2, ctx.m_index);
    assert_int_equal('!', ctx.m_buf[ctx.m_index]);
}

static void rdp_match(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "foo", 4, 0, NULL, NULL};
    assert_true(prometheus_process_linux_limits_rdp_next_token(&ctx) == 0);
    assert_true(prometheus_process_linux_limits_rdp_match(&ctx, "foo"));
}

static void rdp_hard_limit(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "unlimited   ", 13, 0, NULL, NULL};
    
    struct prometheus_process_linux_limits_row row;
    prometheus_process_linux_limits_row_init(env->m_env->m_provider, &row);

    assert_true(prometheus_process_linux_limits_rdp_hard_limit(&ctx, &row));
    assert_int_equal(-1, row.hard);

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);

    /* Test int value*/
    ctx.m_buf = "123  ";
    ctx.m_size = 6;
    ctx.m_index = 0;

    assert_true(prometheus_process_linux_limits_rdp_hard_limit(&ctx, &row));
    assert_int_equal(123, row.hard);

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);
}

static void rdp_word(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    /* Test unlimited value */
    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "unlimited   ", 13, 0, NULL, NULL};

    struct prometheus_process_linux_limits_row row;
    prometheus_process_linux_limits_row_init(env->m_env->m_provider, &row);

    assert_true(prometheus_process_linux_limits_rdp_word(&ctx, &row));
    assert_int_equal(9, ctx.m_index);

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);
}

static void rdp_word_and_space(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "foo bar", 8, 0, NULL, NULL};

    struct prometheus_process_linux_limits_row row;
    prometheus_process_linux_limits_row_init(env->m_env->m_provider, &row);

    assert_true(prometheus_process_linux_limits_rdp_word_and_space(&ctx, &row));
    assert_int_equal(4, ctx.m_index);
    assert_int_equal('b', ctx.m_buf[ctx.m_index]);

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);
}

static void rdp_limit(void **state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "Max cpu time   ", 16, 0, NULL, NULL};

    struct prometheus_process_linux_limits_row row;
    prometheus_process_linux_limits_row_init(env->m_env->m_provider, &row);

    assert_true(prometheus_process_linux_limits_rdp_limit(&ctx,&row));
    assert_int_equal(12, ctx.m_index);
    assert_int_equal(' ', ctx.m_buf[ctx.m_index]);
    assert_string_equal("Max cpu time", row.limit);

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);
}

static void rdp_letter(void ** state) {
    prometheus_process_linux_limits_testenv_t env = *state;

    struct prometheus_process_linux_limits_parse_ctx ctx = {env->m_env->m_provider, "foo", 4, 0, NULL, NULL};

    struct prometheus_process_linux_limits_row row;
    prometheus_process_linux_limits_row_init(env->m_env->m_provider, &row);

    assert_true(prometheus_process_linux_limits_rdp_letter(&ctx, &row));

    ctx.m_size = 1;
    ctx.m_index = 0;
    ctx.m_buf = "";
    assert_false(prometheus_process_linux_limits_rdp_letter(&ctx,&row));

    ctx.m_size = 2;
    ctx.m_index = 0;
    ctx.m_buf = "2";
    assert_false(prometheus_process_linux_limits_rdp_letter(&ctx, &row));

    prometheus_process_linux_limits_row_fini(env->m_env->m_provider, &row);
}

int prometheus_process_linux_limits_basic_tests() {
	const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(rdp_next_token, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_match, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_hard_limit, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_word, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_word_and_space, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_limit, setup, teardown),
        cmocka_unit_test_setup_teardown(rdp_letter, setup, teardown),
        cmocka_unit_test_setup_teardown(file_parsing, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
