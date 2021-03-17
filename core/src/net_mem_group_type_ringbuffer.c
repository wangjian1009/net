#include <assert.h>
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_mem_group_type_ringbuffer_i.h"
#include "net_mem_group_type_i.h"
#include "net_mem_block_i.h"
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

static net_mem_group_type_ringbuffer_block_t
net_mem_group_type_ringbuffer_do_alloc(net_mem_group_type_ringbuffer_t rb, int total_size , int size) {
	net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, rb->head);
	int align_length = ALIGN(sizeof(struct net_mem_group_type_ringbuffer_block) + size);
	blk->length = sizeof(struct net_mem_group_type_ringbuffer_block) + size;
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
	for (i=0; i<2; i++) {
		int free_size = 0;
		net_mem_group_type_ringbuffer_block_t blk = block_ptr(rb, rb->head);
		do {
			if (blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block) && blk->id >= 0)
				return NULL;
			free_size += ALIGN(blk->length);
			if (free_size >= align_length) {
				return net_mem_group_type_ringbuffer_do_alloc(rb, free_size , size);
			}
			blk = block_next(rb, blk);
		} while(blk);
		rb->head = 0;
	}
	return NULL;
}

static int net_mem_group_type_ringbuffer_last_id(net_mem_group_type_ringbuffer_t rb) {
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

void net_mem_group_type_ringbuffer_shrink(
    net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int size)
{
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

static int net_mem_group_type_ringbuffer_block_id(net_mem_group_type_ringbuffer_block_t blk) {
	assert(blk->length >= sizeof(struct net_mem_group_type_ringbuffer_block));
	int id = blk->id;
	assert(id>=0);
	return id;
}

/* net_mem_group_type_ringbuffer_block_t */
/* net_mem_group_type_ringbuffer_yield( */
/*     net_mem_group_type_ringbuffer_t rb, net_mem_group_type_ringbuffer_block_t blk, int skip) */
/* { */
/* 	int length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block) - blk->offset; */
/* 	for (;;) { */
/* 		if (length > skip) { */
/* 			blk->offset += skip; */
/* 			return blk; */
/* 		} */
/* 		blk->id = -1; */
/* 		if (blk->next < 0) { */
/* 			return NULL; */
/* 		} */
/* 		blk = block_ptr(rb, blk->next); */
/* 		assert(blk->offset == 0); */
/* 		skip -= length; */
/* 		length = blk->length - sizeof(struct net_mem_group_type_ringbuffer_block); */
/* 	} */
/* } */

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
				stream_printf(ws, " next=%d", blk->next);
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
    net_mem_group_type_t type, uint32_t ep_id, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy)
{
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

    int pre_collect_id = -1;
    net_mem_group_type_ringbuffer_block_t blk = net_mem_group_type_ringbuffer_alloc(rb , *capacity);
	while (blk == NULL) {
		int collect_id = net_mem_group_type_ringbuffer_last_id(rb);

        net_endpoint_t old_ep = net_endpoint_find(schedule, collect_id);
        if (old_ep == NULL) {
            CPE_ERROR(
                schedule->m_em, "core: ringbuffer: endpoint %d: block owner of %d not exist",
                ep_id, collect_id);
            return NULL;
        }

		if (ep_id == collect_id) {
            CPE_ERROR(
                schedule->m_em, "core: ringbuffer: endpoint %d: ep %s: state=%s, block by self, buf-size=%d",
                ep_id, net_endpoint_dump(net_schedule_tmp_buffer(schedule), old_ep),
                net_endpoint_state_str(net_endpoint_state(old_ep)),
                net_endpoint_buf_size_total(old_ep));
            return NULL;
        }

        /*endpont.m_tp是不会被动释放的，所以可能导致被tp阻塞，这种情况下需要确保被同一个ep占用导致死循环 */
        if (collect_id == pre_collect_id) {
            CPE_ERROR(
                schedule->m_em, "core: ringbuffer: endpoint %d: ep %s: state=%s, block-again, tmp-buf=%d",
                ep_id, net_endpoint_dump(net_schedule_tmp_buffer(schedule), old_ep),
                net_endpoint_state_str(net_endpoint_state(old_ep)),
                old_ep->m_tb ? old_ep->m_tb->m_capacity : 0);
            return NULL;
        }
        
        if (!net_endpoint_is_active(old_ep)) {
            CPE_ERROR(
                schedule->m_em, "core: ringbuffer: endpoint %d: old ep %s state=%s, still have data",
                ep_id, net_endpoint_dump(net_schedule_tmp_buffer(schedule), old_ep),
                net_endpoint_state_str(net_endpoint_state(old_ep)));
            return NULL;
        }

        if (!net_endpoint_have_error(old_ep)) {
            net_endpoint_set_error(
                old_ep,
                net_endpoint_error_source_network, net_endpoint_network_errno_internal, "ringbuffer-block-retrieve");
        }
        if (net_endpoint_set_state(old_ep, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(old_ep, net_endpoint_state_deleting);
        }

		blk = net_mem_group_type_ringbuffer_alloc(rb , *capacity);
    }

    return blk ? (blk + 1) : NULL;
}

void net_mem_group_type_ringbuffer_block_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

    net_mem_group_type_ringbuffer_block_t blk = ((net_mem_group_type_ringbuffer_block_t)data) - 1;

	int id = net_mem_group_type_ringbuffer_block_id(blk);
	blk->id = -1;
	while (blk->next >= 0) {
		blk = block_ptr(rb, blk->next);
		assert(net_mem_group_type_ringbuffer_block_id(blk) == id);
		blk->id = -1;
	}
}

void net_mem_group_type_ringbuffer_block_update_ep(net_mem_group_type_t type, uint32_t ep_id, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_ringbuffer_t rb = net_mem_group_type_data(type);

    net_mem_group_type_ringbuffer_block_t blk = ((net_mem_group_type_ringbuffer_block_t)data) - 1;
    assert(blk->id != -1);
    blk->id = ep_id;
}

net_mem_group_type_t
net_mem_group_type_ringbuffer_create(net_schedule_t schedule, uint64_t size) {
    net_mem_group_type_t type =
        net_mem_group_type_create(
            schedule, net_mem_type_ringbuffer,
            sizeof(struct net_mem_group_type_ringbuffer),
            net_mem_group_type_ringbuffer_init, net_mem_group_type_ringbuffer_fini,
            net_mem_group_type_ringbuffer_suggest_size,
            net_mem_group_type_ringbuffer_block_alloc,
            net_mem_group_type_ringbuffer_block_free,
            net_mem_group_type_ringbuffer_block_update_ep);
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
