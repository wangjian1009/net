#ifndef NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_RINGBUFFER_I_H_INCLEDED
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_mem_group_type_ringbuffer * net_mem_group_type_ringbuffer_t;

struct ringbuffer_block {
	int length;
	int offset;
	int id;
	int next;
};

/**/
struct net_mem_group_type_ringbuffer {
	int size;
	int head;
    void * m_data;
};

net_mem_group_type_t net_mem_group_type_ringbuffer_create(net_schedule_t schedule);

/**/
void ringbuffer_link(net_mem_group_type_ringbuffer_t rb , struct ringbuffer_block * prev, struct ringbuffer_block * next);
struct ringbuffer_block * ringbuffer_alloc(net_mem_group_type_ringbuffer_t rb, int size);
int ringbuffer_collect(net_mem_group_type_ringbuffer_t rb);
void ringbuffer_shrink(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block * blk, int size);
void ringbuffer_free(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block * blk);
int ringbuffer_data(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block * blk, int size, int skip, void **ptr);
void * ringbuffer_copy(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block * from, int skip, struct ringbuffer_block * to);
struct ringbuffer_block * ringbuffer_yield(net_mem_group_type_ringbuffer_t rb, struct ringbuffer_block *blk, int skip);

void net_mem_group_type_ringbuffer_print(write_stream_t ws, net_mem_group_type_ringbuffer_t rb);
const char * net_mem_group_type_ringbuffer_dump(mem_buffer_t buffer, net_mem_group_type_ringbuffer_t rb);

NET_END_DECL

#endif
