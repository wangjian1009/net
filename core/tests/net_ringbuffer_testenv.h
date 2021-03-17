#ifndef TESTS_NET_CORE_RINGBUFFER_TESTENV_H_INCLEDED
#define TESTS_NET_CORE_RINGBUFFER_TESTENV_H_INCLEDED
#include "net_core_testenv.h"
#include "net_mem_group_type_ringbuffer_i.h"

typedef struct net_ringbuffer_testenv * net_ringbuffer_testenv_t;

struct net_ringbuffer_testenv {
    net_core_testenv_t m_env;
    net_mem_group_type_ringbuffer_t m_rb;
    net_mem_group_t m_mem_group;
};

net_ringbuffer_testenv_t net_ringbuffer_testenv_create();
void net_ringbuffer_testenv_free(net_ringbuffer_testenv_t env);

void net_ringbuffer_testenv_create_rb(net_ringbuffer_testenv_t env, uint64_t size);
net_endpoint_t net_ringbuffer_testenv_create_ep(net_ringbuffer_testenv_t env);

#endif
