#ifndef NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_mem_group_type_ringbuffer * net_mem_group_type_ringbuffer_t;
typedef struct net_mem_group_type_ringbuffer_block * net_mem_group_type_ringbuffer_block_t;

struct net_mem_group_type_ringbuffer_block {
	int length;
	int id;
	int next;
};

/**/
struct net_mem_group_type_ringbuffer {
	int size;
	int head;
    void * m_data;
};

net_mem_group_type_t net_mem_group_type_ringbuffer_create(net_schedule_t schedule, uint64_t size);

/**/
void net_mem_group_type_ringbuffer_shrink(
    net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int size);

void net_mem_group_type_ringbuffer_print(write_stream_t ws, net_mem_group_type_ringbuffer_t rb);
const char * net_mem_group_type_ringbuffer_dump(mem_buffer_t buffer, net_mem_group_type_ringbuffer_t rb);

NET_END_DECL

#endif
