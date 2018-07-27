#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/ringbuffer.h"
#include "net_endpoint_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_protocol_i.h"

#define net_endpoint_rbuf_link(__buf)               \
    do {                                                \
        if (endpoint->m_rb) {                           \
            (__buf)->id = -1;                           \
            ringbuffer_link(schedule->m_endpoint_buf,   \
                            endpoint->m_rb, (__buf));   \
        }                                               \
        else {                                          \
            (__buf)->id = endpoint->m_id;               \
            endpoint->m_rb = (__buf);                   \
        }                                               \
    } while(0)


uint8_t net_endpoint_rbuf_is_full(net_endpoint_t endpoint) {
    return 0;
}

uint8_t net_endpoint_rbuf_is_empty(net_endpoint_t endpoint) {
    return endpoint->m_rb ? 0 : 1;
}

void * net_endpoint_rbuf_alloc(net_endpoint_t endpoint, uint32_t * inout_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    void * data;
    int size;

    assert(inout_size);
    assert(*inout_size >= 0);

    if (schedule->m_endpoint_tb) {
        size = ringbuffer_block_data(schedule->m_endpoint_buf, schedule->m_endpoint_tb, 0, &data);
        if (size > 0 && (*inout_size == 0 ||  (uint32_t)size > *inout_size)) {
            if (*inout_size == 0) *inout_size = (uint32_t)size;
            assert(data);
            return data;
        }
        else {
            ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
            schedule->m_endpoint_tb = NULL;
        }
    }

    assert(schedule->m_endpoint_tb == NULL);

    size = (int)*inout_size;
    if (size == 0) {
        size = 4 * 1024;
    }

    schedule->m_endpoint_tb = net_endpoint_common_buf_alloc(endpoint, (uint32_t)size);
    if (schedule->m_endpoint_tb == NULL) {
        return NULL;
    }

    size = ringbuffer_block_data(schedule->m_endpoint_buf, schedule->m_endpoint_tb, 0, &data);
    assert(data);
    *inout_size = (uint32_t)size;

    return data;
}

int net_endpoint_rbuf_supply(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(schedule->m_endpoint_tb);

    if (size > 0) {
        ringbuffer_shrink(schedule->m_endpoint_buf, schedule->m_endpoint_tb, (int)size);
        net_endpoint_rbuf_link(schedule->m_endpoint_tb);
    }
    else {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
    }
    
    schedule->m_endpoint_tb = NULL;

    return endpoint->m_protocol->m_endpoint_input(endpoint);
}

uint32_t net_endpoint_rbuf_size(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_rb) {
        return ringbuffer_block_len(schedule->m_endpoint_buf, endpoint->m_rb, 0);
    }
    else {
        return 0;
    }
}

void net_endpoint_rbuf_consume(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (schedule->m_endpoint_tb) {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
        schedule->m_endpoint_tb = NULL;
    }

    if (endpoint->m_rb) {
        endpoint->m_rb = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_rb, size);

        if (schedule->m_data_monitor_fun) {
            schedule->m_data_monitor_fun(
                schedule->m_data_monitor_ctx, endpoint, net_data_in, size);
        }
    }
}

int net_endpoint_rbuf(net_endpoint_t endpoint, uint32_t require, void * * r_data) {
    if (endpoint->m_rb == NULL) {
        *r_data = NULL;
        return 0;
    }

    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    void * data;
    int size = ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_rb, 0, &data);
    if (size >= require) {
        *r_data = data;
        return 0;
    }

    size = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_rb);
    if (size >= require) {
        ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)size);
        if (combine_blk == NULL) return -1;

        *r_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_rb, 0, combine_blk);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_rb);
        combine_blk->id = endpoint->m_id;
        endpoint->m_rb = combine_blk;
        return 0;
    }
    else {
        *r_data = NULL;
        return 0;
    }
}

int net_endpoint_rbuf_recv(net_endpoint_t endpoint, void * data, uint32_t * size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    uint32_t capacity = *size;
    uint32_t received = 0;

    while(capacity > 0 && endpoint->m_rb != NULL) {
        char * block_data;
        uint32_t block_data_len = (uint32_t)ringbuffer_block_len(schedule->m_endpoint_buf, endpoint->m_rb, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_rb, 0, (void**)&block_data);

        uint32_t copy_len = block_data_len > capacity ? capacity : block_data_len;
        memcpy(data, block_data, copy_len);

        capacity -= copy_len;
        data += copy_len;
        received += copy_len;
        
        endpoint->m_rb = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_rb, (int)copy_len);
    }

    *size = received;

    if (received) {
        if (schedule->m_data_monitor_fun) {
            schedule->m_data_monitor_fun(
                schedule->m_data_monitor_ctx, endpoint, net_data_in, received);
        }
    }
    
    return 0;
}

int net_endpoint_rbuf_by_sep(net_endpoint_t endpoint, const char * seps, void * * r_data, uint32_t *r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    uint32_t sz = 0;
    ringbuffer_block_t block = endpoint->m_rb;
    while(block) {
        char * block_data;
        int block_data_len = ringbuffer_block_len(schedule->m_endpoint_buf, block, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, block, 0, (void**)&block_data);

        int block_pos;
        for(block_pos = 0; block_pos < block_data_len; ++block_pos) {
            int j;
            for(j = 0; seps[j] != 0; ++j) {
                if (block_data[block_pos] == seps[j]) {
                    if (block != endpoint->m_rb) { /*多块，需要合并 */
                        block_pos += sz;
                        block_data_len = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_rb);
                        ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)block_data_len);
                        if (combine_blk == NULL) return -1;

                        block_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_rb, 0, combine_blk);
                        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_rb);
                        combine_blk->id = endpoint->m_id;
                        endpoint->m_rb = combine_blk;
                    }

                    block_data[block_pos] = 0;
                    *r_data = block_data;
                    *r_size = block_pos + 1;
                    return 0;
                }
            }
        }
        sz += (uint32_t)block_data_len;
        block = ringbuffer_block_link_next(schedule->m_endpoint_buf, block);
    }
    
    *r_data = NULL;
    *r_size = 0;
    return 0;
}

static uint8_t net_endpoint_rbuf_match_forward(
    net_schedule_t schedule, ringbuffer_block_t block, char * block_data, int block_data_len, int block_pos,
    const char * look_str, size_t look_str_len)
{
    int i;
    for(i = 0; i < 100; ++i) {
        assert(block_data_len > block_pos);

        size_t cmp_sz = (size_t)(block_data_len - block_pos);
        if (cmp_sz > look_str_len) cmp_sz = look_str_len;

        if (memcmp(block_data + block_pos, look_str, cmp_sz) != 0) return 0;

        look_str += cmp_sz;
        look_str_len -= cmp_sz;

        if (look_str_len == 0) return 1;

        block = ringbuffer_block_link_next(schedule->m_endpoint_buf, block);
        if (block == NULL) return 0;

        block_data_len = ringbuffer_block_len(schedule->m_endpoint_buf, block, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, block, 0, (void**)&block_data);

        block_pos = 0;
    }

    assert(0);
    return 0;
}

int net_endpoint_rbuf_by_str(net_endpoint_t endpoint, const char * str, void * * r_data, uint32_t * r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    size_t str_len = strlen(str);
    if (str_len == 0) return -1;

    uint32_t sz = 0;
    ringbuffer_block_t block = endpoint->m_rb;
    while(block) {
        char * block_data;
        int block_data_len = ringbuffer_block_len(schedule->m_endpoint_buf, block, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, block, 0, (void**)&block_data);

        int block_pos;
        for(block_pos = 0; block_pos < block_data_len; ++block_pos) {
            if (block_data[block_pos] == str[0]
                && net_endpoint_rbuf_match_forward(
                    schedule, block, block_data, block_data_len, block_pos + 1, str + 1, str_len - 1)
                )
            {
                if (block != endpoint->m_rb /*已经涉及多个块*/
                    || (block_pos + str_len) > block_data_len /**/)
                {
                    /*多块，需要合并 */
                    block_pos += sz;
                    block_data_len = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_rb);
                    ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)block_data_len);
                    if (combine_blk == NULL) return -1;

                    block_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_rb, 0, combine_blk);
                    ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_rb);
                    combine_blk->id = endpoint->m_id;
                    endpoint->m_rb = combine_blk;
                }

                block_data[block_pos] = 0;
                *r_data = block_data;
                *r_size = block_pos + str_len;
                return 0;
            }
        }
        sz += (uint32_t)block_data_len;
        block = ringbuffer_block_link_next(schedule->m_endpoint_buf, block);
    }
    
    *r_data = NULL;
    *r_size = 0;
    return 0;
}
