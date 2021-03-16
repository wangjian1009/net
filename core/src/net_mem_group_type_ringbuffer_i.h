#ifndef NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#include "cpe/utils/ringbuffer.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_mem_group_type_ringbuffer * net_mem_group_type_ringbuffer_t;

struct net_mem_group_type_ringbuffer {
    uint32_t m_alloced_count;
    uint64_t m_alloced_size;
    uint8_t m_group_count;
    ringbuffer_t m_ring_buffer;
};

net_mem_group_type_t net_mem_group_type_cache_create(net_schedule_t schedule);

NET_END_DECL

#endif
