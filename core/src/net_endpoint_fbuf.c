#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/ringbuffer.h"
#include "net_endpoint_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_protocol_i.h"

#define net_endpoint_fbuf_link(__buf)               \
    do {                                                \
        if (endpoint->m_fb) {                           \
            (__buf)->id = -1;                           \
            ringbuffer_link(schedule->m_endpoint_buf,   \
                            endpoint->m_fb, (__buf));   \
        }                                               \
        else {                                          \
            (__buf)->id = endpoint->m_id;               \
            endpoint->m_fb = (__buf);                   \
        }                                               \
        assert(endpoint->m_fb->id == endpoint->m_id);   \
    } while(0)

int net_endpoint_fbuf_append(net_endpoint_t endpoint, void const * i_data, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (size == 0) return 0;

    ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
    if (tb == NULL) return -1;

    void * data;
    ringbuffer_block_data(schedule->m_endpoint_buf, tb, 0, &data);
    assert(data);

    memcpy(data, i_data, size);

    ringbuffer_shrink(schedule->m_endpoint_buf, tb, (int)size);

    net_endpoint_fbuf_link(tb);

    return 0;
}

int net_endpoint_fbuf_append_from_rbuf(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    if (size > 0) {
        uint32_t rbuf_len = endpoint->m_rb ? ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_rb) : 0;
        if (size > rbuf_len) {
            CPE_ERROR(
                schedule->m_em, "core: %s: rbuf ==> fbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                rbuf_len, size);
            return -1;
        }
        
        ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
        if (tb == NULL) return -1;
        
        ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_rb, 0, tb);

        net_endpoint_fbuf_link(tb);

        net_endpoint_rbuf_consume(endpoint, size);
    }
    else {
        if (endpoint->m_rb) {
            net_endpoint_fbuf_link(endpoint->m_rb);
            endpoint->m_rb = NULL;
        }
    }

    return 0;
}

uint32_t net_endpoint_fbuf_size(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_fb) {
		return ringbuffer_block_len(schedule->m_endpoint_buf, endpoint->m_fb, 0);
	}
    else {
        return 0;
	}
}

void net_endpoint_fbuf_consume(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    endpoint->m_fb = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_fb, size);
    assert(endpoint->m_fb == NULL || endpoint->m_fb->id == endpoint->m_id);
}

int net_endpoint_fbuf(net_endpoint_t endpoint, uint32_t require, void * * r_data) {
    if (endpoint->m_fb == NULL) {
        *r_data = NULL;
        return 0;
    }

    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    void * data;
    int size = ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_fb, 0, &data);
    if (size >= require) {
        *r_data = data;
        return 0;
    }

    size = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_fb);
    if (size >= require) {
        ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)size);
        if (combine_blk == NULL) return -1;

        *r_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_fb, 0, combine_blk);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_fb);
        combine_blk->id = endpoint->m_id;
        endpoint->m_fb = combine_blk;
    }
    else {
        *r_data = NULL;
    }

    return 0;
}
