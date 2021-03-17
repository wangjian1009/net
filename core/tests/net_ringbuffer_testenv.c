#include "net_endpoint.h"
#include "net_mem_group.h"
#include "net_mem_group_type_i.h"
#include "net_ringbuffer_testenv.h"

net_ringbuffer_testenv_t net_ringbuffer_testenv_create() {
    net_ringbuffer_testenv_t env = mem_alloc(test_allocrator(), sizeof(struct net_ringbuffer_testenv));
    env->m_env = net_core_testenv_create();
    env->m_rb = NULL;
    return env;
}

void net_ringbuffer_testenv_free(net_ringbuffer_testenv_t env) {
    net_core_testenv_free(env->m_env);
    mem_free(test_allocrator(), env);
}

void net_ringbuffer_testenv_create_rb(net_ringbuffer_testenv_t env, uint64_t size) {
    net_mem_group_type_t type = net_mem_group_type_ringbuffer_create(env->m_env->m_schedule, size);
    env->m_rb = net_mem_group_type_data(type);
    env->m_mem_group = net_mem_group_create(type);
}

net_endpoint_t net_ringbuffer_testenv_create_ep(net_ringbuffer_testenv_t env) {
    net_endpoint_t ep = net_endpoint_create(
        net_driver_from_data(env->m_env->m_tdriver),
        net_protocol_from_data(env->m_env->m_tprotocol),
        env->m_mem_group);

    return ep;
}

/* static void init(net_mem_group_type_ringbuffer_block_t blk, int n) { */
/* 	char * ptr = (char *)(blk+1); */
/* 	int i; */
/* 	for (i=0;i<n;i++) { */
/* 		ptr[i] = i + 1; */
/* 	} */
/* } */

/* static void dump(net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int size) { */
/* 	void * buffer; */
/* 	int sz = net_mem_group_type_ringbuffer_data(rb, blk, size, 0, &buffer); */
/* 	char * data = buffer; */
/* 	if (data) { */
/* 		int i; */
/* 		for (i=0;i<sz;i++) { */
/* 			printf("%d ",data[i]); */
/* 		} */
/* 		printf("\n"); */
/* 	} else { */
/* 		printf("size = %d\n",sz); */
/* 	} */
/* } */

