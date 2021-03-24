#include "cmocka_all.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "prometheus_process_tests.h"
#include "prometheus_process_testenv.h"
#include "prometheus_process_limits_i.h"

static int setup(void **state) {
    prometheus_process_testenv_t env = prometheus_process_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    prometheus_process_testenv_t env = *state;
    prometheus_process_testenv_free(env);
    return 0;
}

static void file_parsing(void **state) {
    prometheus_process_testenv_t env = *state;
    
    prometheus_process_install_limits(
        env,
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

  /* prom_process_limits_file_t *f = prom_process_limits_file_new(path); */
  /* char *result = strstr(f->buf, "Max realtime timeout"); */
  /* TEST_ASSERT_NOT_NULL(result); */

  /* prom_map_t *m = prom_process_limits(f); */
  /* if (!m) TEST_FAIL(); */

  /* TEST_ASSERT_EQUAL_INT(16, prom_map_size(m)); */

  /* prom_process_limits_row_t *row = (prom_process_limits_row_t *)prom_map_get(m, "Max cpu time"); */
  /* if (!row) TEST_FAIL(); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max cpu time", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("seconds", row->units); */

  /* row = (prom_process_limits_row_t *)prom_map_get(m, "Max file size"); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max file size", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("bytes", row->units); */

  /* row = (prom_process_limits_row_t *)prom_map_get(m, "Max data size"); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max data size", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("bytes", row->units); */

  /* row = (prom_process_limits_row_t *)prom_map_get(m, "Max stack size"); */
  /* TEST_ASSERT_EQUAL_INT(8388608, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max stack size", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("bytes", row->units); */

  /* row = (prom_process_limits_row_t *)prom_map_get(m, "Max processes"); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(-1, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max processes", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("processes", row->units); */

  /* row = (prom_process_limits_row_t *)prom_map_get(m, "Max pending signals"); */
  /* TEST_ASSERT_EQUAL_INT(23701, row->soft); */
  /* TEST_ASSERT_EQUAL_INT(23701, row->hard); */
  /* TEST_ASSERT_EQUAL_STRING("Max pending signals", row->limit); */
  /* TEST_ASSERT_EQUAL_STRING("signals", row->units); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_file_destroy(f); */
  /* f = NULL; */
}

static void rdp_next_token(void **state) {
  /* prom_process_limits_file_t f = {.size = 4, .index = 0, .buf = " \t!"}; */
  /* prom_process_limits_file_t *fp = &f; */
  /* prom_process_limits_rdp_next_token(fp); */

  /* TEST_ASSERT_EQUAL_INT(2, fp->index); */
  /* TEST_ASSERT_EQUAL_INT('!', fp->buf[fp->index]); */
}

static void rdp_match(void **state) {
  /* prom_process_limits_file_t f = {.size = 4, .index = 0, .buf = "foo"}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_match(fp, "foo")); */
}

static void rdp_hard_limit(void **state) {
  // Test unlimited value
  /* prom_process_limits_file_t f = {.size = 13, .index = 0, .buf = "unlimited   "}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* prom_process_limits_current_row_t *cr = prom_process_limits_current_row_new(); */
  /* prom_map_t *m = prom_map_new(); */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_hard_limit(fp, m, cr)); */
  /* TEST_ASSERT_EQUAL_INT(-1, cr->hard); */

  /* prom_process_limits_current_row_clear(cr); */

  /* // Test int value */
  /* fp->buf = "123  "; */
  /* fp->size = 6; */
  /* fp->index = 0; */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_hard_limit(fp, m, cr)); */
  /* TEST_ASSERT_EQUAL_INT(123, cr->hard); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_current_row_destroy(cr); */
  /* cr = NULL; */
}

static void rdp_word(void **state) {
  /* // Test unlimited value */
  /* prom_process_limits_file_t f = {.size = 13, .index = 0, .buf = "unlimited   "}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* prom_process_limits_current_row_t *cr = prom_process_limits_current_row_new(); */
  /* prom_map_t *m = prom_map_new(); */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_word(fp, m, cr)); */
  /* TEST_ASSERT_EQUAL_INT(9, fp->index); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_current_row_destroy(cr); */
  /* cr = NULL; */
}

static void rdp_word_and_space(void **state) {
  /* prom_process_limits_file_t f = {.size = 8, .index = 0, .buf = "foo bar"}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* prom_process_limits_current_row_t *cr = prom_process_limits_current_row_new(); */
  /* prom_map_t *m = prom_map_new(); */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_word_and_space(fp, m, cr)); */
  /* TEST_ASSERT_EQUAL_INT(4, fp->index); */
  /* TEST_ASSERT_EQUAL_INT('b', fp->buf[fp->index]); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_current_row_destroy(cr); */
  /* cr = NULL; */
}

static void rdp_limit(void **state) {
  /* prom_process_limits_file_t f = {.size = 16, .index = 0, .buf = "Max cpu time   "}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* prom_process_limits_current_row_t *cr = prom_process_limits_current_row_new(); */
  /* prom_map_t *m = prom_map_new(); */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_limit(fp, m, cr)); */
  /* TEST_ASSERT_EQUAL_INT(12, fp->index); */
  /* TEST_ASSERT_EQUAL_INT(' ', fp->buf[fp->index]); */
  /* TEST_ASSERT_EQUAL_STRING("Max cpu time", cr->limit); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_current_row_destroy(cr); */
  /* cr = NULL; */
}

static void rdp_letter(void ** state) {
  /* prom_process_limits_file_t f = {.size = 4, .index = 0, .buf = "foo"}; */
  /* prom_process_limits_file_t *fp = &f; */

  /* prom_process_limits_current_row_t *cr = prom_process_limits_current_row_new(); */
  /* prom_map_t *m = prom_map_new(); */

  /* TEST_ASSERT_TRUE(prom_process_limits_rdp_letter(fp, m, cr)); */

  /* fp->size = 1; */
  /* fp->index = 0; */
  /* fp->buf = ""; */

  /* TEST_ASSERT_FALSE(prom_process_limits_rdp_letter(fp, m, cr)); */

  /* fp->size = 2; */
  /* fp->index = 0; */
  /* fp->buf = "2"; */

  /* TEST_ASSERT_FALSE(prom_process_limits_rdp_letter(fp, m, cr)); */

  /* prom_map_destroy(m); */
  /* m = NULL; */
  /* prom_process_limits_current_row_destroy(cr); */
  /* cr = NULL; */
}

int prometheus_process_limits_basic_tests() {
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
