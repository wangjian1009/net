#include "cmocka_all.h"
#include "net_core_tests.h"
#include "net_ringbuffer_testenv.h"
#include "net_mem_group_type_ringbuffer_i.h"

static int setup(void **state) {
    net_ringbuffer_testenv_t env = net_ringbuffer_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ringbuffer_testenv_t env = *state;
    net_ringbuffer_testenv_free(env);
    return 0;
}

static void ringbuffer_basic(void **state) {
    net_ringbuffer_testenv_t env = *state;
    net_ringbuffer_testenv_create_rb(env, 128);
    
	/* net_mem_group_type_ringbuffer_block_t blk; */
	/* blk = net_mem_group_type_ringbuffer_alloc(rb, 48); */
	/* blk->id = 0; */
	/* net_mem_group_type_ringbuffer_free(rb,blk); */
	/* blk = net_mem_group_type_ringbuffer_alloc(rb, 48); */
	/* blk->id = 1; */
	/* net_mem_group_type_ringbuffer_free(rb,blk); */

	/* blk = net_mem_group_type_ringbuffer_alloc(rb, 80); */
	/* blk->id = 0; */
	/* net_mem_group_type_ringbuffer_free(rb,blk); */

	/* blk = net_mem_group_type_ringbuffer_alloc(rb, 50); */
	/* blk->id = 1; */
	/* net_mem_group_type_ringbuffer_block_t next = net_mem_group_type_ringbuffer_alloc(rb, 40); */
	/* next->id = 1; */
	/* net_mem_group_type_ringbuffer_link(rb, blk, next); */
	/* CPE_INFO(env->m_em, "%s", net_mem_group_type_ringbuffer_dump(net_schedule_tmp_buffer(env->m_schedule), rb)); */

	/* blk = net_mem_group_type_ringbuffer_alloc(rb,4); */
	/* CPE_ERROR(env->m_em, "%p", blk); */

	/* blk = net_mem_group_type_ringbuffer_alloc(rb,4); */
	/* blk->id = 2; */
	/* init(blk,4); */

	/* next = net_mem_group_type_ringbuffer_alloc(rb,5); */
	/* init(next,5); */
	/* net_mem_group_type_ringbuffer_link(rb, blk, next); */

	/* next = net_mem_group_type_ringbuffer_alloc(rb,6); */
	/* init(next,6); */
	/* net_mem_group_type_ringbuffer_link(rb, blk , next); */


	/* dump(rb, blk , 3); */
	/* dump(rb, blk , 6); */
	/* dump(rb, blk , 16); */

	/* blk = net_mem_group_type_ringbuffer_yield(rb, blk, 5); */

	/* next = net_mem_group_type_ringbuffer_alloc(rb, 7); */
	/* net_mem_group_type_ringbuffer_copy(rb, blk, 1, next); */
	/* dump(rb, next, 7); */

	/* blk = net_mem_group_type_ringbuffer_yield(rb, blk , 5); */

    /* CPE_INFO(env->m_em, "%s", net_mem_group_type_ringbuffer_dump(net_schedule_tmp_buffer(env->m_schedule), rb)); */
}

int net_ringbuffer_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(ringbuffer_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
