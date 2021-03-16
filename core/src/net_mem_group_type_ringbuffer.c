#include <assert.h>
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_mem_group_type_ringbuffer_i.h"
#include "net_mem_group_type_i.h"
#include "net_endpoint_i.h"

#define ALIGN(s) (((s) + 3 ) & ~3)

static inline int
block_offset(net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk) {
	char * start = rb->m_data;
	return (char *)blk - start;
}

static inline net_mem_group_type_ringbuffer_block_t
block_ptr(net_mem_group_type_ringbuffer_t rb, int offset) {
	char * start = rb->m_data;
	return (net_mem_group_type_ringbuffer_block_t)(start + offset);
}

static inline net_mem_group_type_ringbuffer_block_t
block_next(net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk) {
	int align_length = ALIGN(blk->length);
	int head = block_offset(rb, blk);
	if (align_length + head == rb->size) {
		return NULL;
	}
	assert(align_length + head < rb->size);
	return block_ptr(rb, head + align_length);
}

void net_mem_group_type_ringbuffer_link(
    net_mem_group_type_ringbuffer_t rb,
    net_mem_group_type_ringbuffer_block_t head, net_mem_group_type_ringbuffer_block_t next)
{
	while (head->next >=0) {
		head = block_ptr(rb, head->next);
	}
	next->id = head->id;
	head->next = block_offset(rb, next);
}

static net_mem_group_type_ringbuffer_block_t
_alloc(net_mem_group_type_ringbuffer_t rb, int total_size , int size) {
	net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, rb->head);
	int align_length = ALIGN(sizeof(struct net_mem_group_type_ringbuffer_block) + size);
	blk->length = sizeof(struct net_mem_group_type_ringbuffer_block) + size;
	blk->offset = 0;
	blk->next = -1;
	blk->id = -1;
	net_mem_group_type_ringbuffer_block_t next = block_next(rb, blk);
	if (next) {
		rb->head = block_offset(rb, next);
		if (align_length < total_size) {
			next->length = total_size - align_length;
			if (next->length >= sizeof(struct net_mem_group_type_ringbuffer_block)) {
				next->id = -1;
			}
		}
	} else {
		rb->head = 0;
	}
	return blk;
}

net_mem_group_type_ringbuffer_block_t
net_mem_group_type_ringbuffer_alloc(net_mem_group_type_ringbuffer_t rb, int size) {
	int align_length = ALIGN(sizeof(struct net_mem_group_type_ringbuffer_block) + size);
	int i;
	for (i=0;i<2;i++) {
		int free_size = 0;
		net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, rb->head);
		do {
			if (blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block) && blk->id >= 0)
				return NULL;
			free_size += ALIGN(blk->length);
			if (free_size >= align_length) {
				return _alloc(rb, free_size , size);
			}
			blk = block_next(rb, blk);
		} while(blk);
		rb->head = 0;
	}
	return NULL;
}

static int
_last_id(net_mem_group_type_ringbuffer_t rb) {
	int i;
	for (i=0;i<2;i++) {
		net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, rb->head);
		do {
			if (blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block) && blk->id >= 0)
				return blk->id;
			blk = block_next(rb, blk);
		} while(blk);
		rb->head = 0;
	}
	return -1;
}

int
net_mem_group_type_ringbuffer_collect(net_mem_group_type_ringbuffer_t rb) {
	int id = _last_id(rb);
	net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, 0);
	do {
		if (blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block) && blk->id == id) {
			blk->id = -1;
		}
		blk = block_next(rb, blk);
	} while(blk);
	return id;
}

void
net_mem_group_type_ringbuffer_shrink(net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int size) {
	if (size == 0) {
		rb->head = block_offset(rb, blk);
		return;
	}
	int align_length = ALIGN(sizeof(struct net_mem_group_type_ringbuffer_block) + size);
	int old_length = ALIGN(blk->length);
	assert(align_length <= old_length);
	blk->length = size + sizeof(struct net_mem_group_type_ringbuffer_block);
	if (align_length == old_length) {
		return;
	}
	blk = block_next(rb, blk);
	blk->length = old_length - align_length;
	if (blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block)) {
		blk->id = -1;
	}
	rb->head = block_offset(rb, blk);
}

static int
_block_id(net_mem_group_type_ringbuffer_block_t blk) {
	assert(blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block));
	int id = blk->id;
	assert(id>=0);
	return id;
}

void
net_mem_group_type_ringbuffer_free(net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk) {
	if (blk == NULL)
		return;
	int id = _block_id(blk);
	blk->id = -1;
	while (blk->next >= 0) {
		blk = block_ptr(rb, blk->next);
		assert(_block_id(blk) == id);
		blk->id = -1;
	}
}

int net_mem_group_type_ringbuffer_data(
    net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int size, int skip, void **ptr)
{
	int length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block) - blk->offset;
	for (;;) {
		if (length > skip) {
			if (length - skip >= size) {
				char * start = (char *)(blk + 1);
				*ptr = (start + blk->offset + skip);
				return size;
			}
			*ptr = NULL;
			int ret = length - skip;
			while (blk->next >= 0) {
				blk = block_ptr(rb, blk->next);
				ret += blk->length - sizeof(struct net_mem_group_type_ringbuffer_block);
				if (ret >= size)
					return size;
			}
			return ret;
		}
		if (blk->next < 0) {
			assert(length == skip);
			*ptr = NULL;
			return 0;
		}
		blk = block_ptr(rb, blk->next);
		assert(blk->offset == 0);
		skip -= length;
		length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block);
	}
}

void *
net_mem_group_type_ringbuffer_copy(
    net_mem_group_type_ringbuffer_t rb,
    net_mem_group_type_ringbuffer_block_t from, int skip, net_mem_group_type_ringbuffer_block_t to)
{
	int size = to->length - sizeof(struct net_mem_group_type_ringbuffer_block);
	int length = from->length - sizeof(struct net_mem_group_type_ringbuffer_block) - from->offset;
	char * ptr = (char *)(to+1);
	for (;;) {
		if (length > skip) {
			char * src = (char *)(from + 1);
			src += from->offset + skip;
			length -= skip;
			while (length < size) {
				memcpy(ptr, src, length);
				assert(from->next >= 0);
				from = block_ptr(rb , from->next);
				assert(from->offset == 0);
				ptr += length;
				size -= length;
				length = from->length - sizeof(struct net_mem_group_type_ringbuffer_block);
				src =  (char *)(from + 1);
			}
			memcpy(ptr, src , size);
			to->id = from->id;
			return (char *)(to + 1);
		}
		assert(from->next >= 0);
		from = block_ptr(rb, from->next);
		assert(from->offset == 0);
		skip -= length;
		length = from->length - sizeof(struct net_mem_group_type_ringbuffer_block);
	}
}

net_mem_group_type_ringbuffer_block_t
net_mem_group_type_ringbuffer_yield(
    net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int skip)
{
	int length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block) - blk->offset;
	for (;;) {
		if (length > skip) {
			blk->offset += skip;
			return blk;
		}
		blk->id = -1;
		if (blk->next < 0) {
			return NULL;
		}
		blk = block_ptr(rb, blk->next);
		assert(blk->offset == 0);
		skip -= length;
		length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block);
	}
}

void net_mem_group_type_ringbuffer_print(write_stream_t ws, net_mem_group_type_ringbuffer_t rb) {
	net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, 0);
	int i=0;
	stream_printf(ws, "total size= %d\n", (int)rb->size);
	while (blk) {
		++i;
		if (i>10)
			break;
		if (blk->length >= sizeof(*blk)) {
			stream_printf(ws, "[%u : %d]", (unsigned)(blk->length - sizeof(*blk)), block_offset(rb,blk));
			stream_printf(ws, " id=%d",blk->id);
			if (blk->id >=0) {
				stream_printf(ws, " offset=%d next=%d",blk->offset, blk->next);
			}
		} else {
			stream_printf(ws, "<%u : %d>", blk->length, block_offset(rb,blk));
		}
		stream_printf(ws, "\n");
		blk = block_next(rb, blk);
	}
}

const char * net_mem_group_type_ringbuffer_dump(mem_buffer_t buffer, net_mem_group_type_ringbuffer_t rb) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    net_mem_group_type_ringbuffer_print((write_stream_t)&stream, rb);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

int net_mem_group_type_ringbuffer_init(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

	rb->size = 0;
	rb->head = 0;
    rb->m_data = NULL;

    return 0;
}

void net_mem_group_type_ringbuffer_fini(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

    if (rb->m_data) {
        mem_free(schedule->m_alloc, rb->m_data);
        rb->m_data = NULL;
    }
}

uint32_t net_mem_group_type_ringbuffer_suggest_size(net_mem_group_type_t type) {
    return 0;
}

void * net_mem_group_type_ringbuffer_block_alloc(
    net_mem_group_type_t type, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy)
{
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);
    return NULL;
}

void net_mem_group_type_ringbuffer_block_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t ringbuffer = net_mem_group_type_data(type);
}

net_mem_group_type_t
net_mem_group_type_ringbuffer_create(net_schedule_t schedule, uint64_t size) {
    net_mem_group_type_t type =
        net_mem_group_type_create(
            schedule, net_mem_type_ringbuffer,
            sizeof(struct net_mem_group_type_ringbuffer),
            net_mem_group_type_ringbuffer_init, net_mem_group_type_ringbuffer_fini,
            net_mem_group_type_ringbuffer_suggest_size,
            net_mem_group_type_ringbuffer_block_alloc, net_mem_group_type_ringbuffer_block_free);
    if (type == NULL) return NULL;

    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

	rb->size = (int)size;
	rb->head = 0;
    rb->m_data = mem_alloc(schedule->m_alloc, rb->size);

	net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, 0);
	blk->length = rb->size;
	blk->id = -1;
    
    return type;
}
