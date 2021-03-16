#include "cmocka_all.h"
#include "net_core_testenv.h"
#include "net_core_tests.h"
#include "net_mem_group_type_i.h"
#include "net_mem_group.h"
#include "net_mem_group_type_ringbuffer_i.h"

static void init(struct ringbuffer_block * blk, int n) {
	char * ptr = (char *)(blk+1);
	int i;
	for (i=0;i<n;i++) {
		ptr[i] = i + 1;
	}
}

static void dump(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block *blk, int size) {
	void * buffer;
	int sz = net_mem_group_type_ringbuffer_data(rb, blk, size, 0, &buffer);
	char * data = buffer;
	if (data) {
		int i;
		for (i=0;i<sz;i++) {
			printf("%d ",data[i]);
		}
		printf("\n");
	} else {
		printf("size = %d\n",sz);
	}
}

static int setup(void **state) {
    net_core_testenv_t env = net_core_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_core_testenv_t env = *state;
    net_core_testenv_free(env);
    return 0;
}

static void ringbuffer_basic(void **state) {
    net_core_testenv_t env = *state;
    net_mem_group_type_t type = net_mem_group_type_ringbuffer_create(env->m_schedule);
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);
    
	struct ringbuffer_block * blk;
	blk = net_mem_group_type_ringbuffer_alloc(rb, 48);
	blk->id = 0;
	net_mem_group_type_ringbuffer_free(rb,blk);
	blk = net_mem_group_type_ringbuffer_alloc(rb, 48);
	blk->id = 1;
	net_mem_group_type_ringbuffer_free(rb,blk);

	blk = net_mem_group_type_ringbuffer_alloc(rb, 80);
	blk->id = 0;
	net_mem_group_type_ringbuffer_free(rb,blk);

	blk = net_mem_group_type_ringbuffer_alloc(rb, 50);
	blk->id = 1;
	struct ringbuffer_block * next = net_mem_group_type_ringbuffer_alloc(rb, 40);
	next->id = 1;
	net_mem_group_type_ringbuffer_link(rb, blk, next);
	CPE_INFO(env->m_em, "%s", net_mem_group_type_ringbuffer_dump(net_schedule_tmp_buffer(env->m_schedule), rb));

	blk = net_mem_group_type_ringbuffer_alloc(rb,4);
	CPE_ERROR(env->m_em, "%p", blk);

	int id = net_mem_group_type_ringbuffer_collect(rb);
	CPE_INFO(env->m_em, "collect %d",id);

	blk = net_mem_group_type_ringbuffer_alloc(rb,4);
	blk->id = 2;
	init(blk,4);

	next = net_mem_group_type_ringbuffer_alloc(rb,5);
	init(next,5);
	net_mem_group_type_ringbuffer_link(rb, blk, next);

	next = net_mem_group_type_ringbuffer_alloc(rb,6);
	init(next,6);
	net_mem_group_type_ringbuffer_link(rb, blk , next);


	dump(rb, blk , 3);
	dump(rb, blk , 6);
	dump(rb, blk , 16);

	blk = net_mem_group_type_ringbuffer_yield(rb, blk, 5);

	next = net_mem_group_type_ringbuffer_alloc(rb, 7);
	net_mem_group_type_ringbuffer_copy(rb, blk, 1, next);
	dump(rb, next, 7);

	blk = net_mem_group_type_ringbuffer_yield(rb, blk , 5);

    CPE_INFO(env->m_em, "%s", net_mem_group_type_ringbuffer_dump(net_schedule_tmp_buffer(env->m_schedule), rb));
}

int net_ringbuffer_basic_tests() {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(ringbuffer_basic, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
