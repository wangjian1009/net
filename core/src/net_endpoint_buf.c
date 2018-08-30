#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/ringbuffer.h"
#include "net_endpoint_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_protocol_i.h"

#define net_endpoint_buf_link(__buf)                                    \
    do {                                                                \
        if (endpoint->m_bufs[buf_type]) {                                           \
            ringbuffer_link(                                            \
                schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], (__buf));     \
            assert(endpoint->m_bufs[buf_type]->id == endpoint->m_id);               \
        }                                                               \
        else {                                                          \
            ringbuffer_block_set_id(                                    \
                schedule->m_endpoint_buf, (__buf), endpoint->m_id);     \
            endpoint->m_bufs[buf_type] = (__buf);                                   \
        }                                                               \
    } while(0)

uint8_t net_endpoint_buf_is_full(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    assert(buf_type < net_ep_buf_count);
    return 0;
}

uint8_t net_endpoint_buf_is_empty(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    assert(buf_type < net_ep_buf_count);
    return endpoint->m_bufs[buf_type] ? 0 : 1;
}

void * net_endpoint_buf_alloc(net_endpoint_t endpoint, uint32_t * inout_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    void * data;
    int size;

    assert(inout_size);
    assert(*inout_size >= 0);

    /* if (schedule->m_endpoint_tb) { */
    /*     size = ringbuffer_block_data(schedule->m_endpoint_buf, schedule->m_endpoint_tb, 0, &data); */
    /*     if (size > 0 && (*inout_size == 0 ||  (uint32_t)size > *inout_size)) { */
    /*         if (*inout_size == 0) *inout_size = (uint32_t)size; */
    /*         assert(data); */
    /*         return data; */
    /*     } */
    /*     else { */
    /*         ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb); */
    /*         schedule->m_endpoint_tb = NULL; */
    /*     } */
    /* } */

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

void net_endpoint_buf_release(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

   assert(schedule->m_endpoint_tb);

   ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
   schedule->m_endpoint_tb = NULL;
}

int net_endpoint_buf_supply(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    assert(schedule->m_endpoint_tb);

    if (size > 0) {
        ringbuffer_shrink(schedule->m_endpoint_buf, schedule->m_endpoint_tb, (int)size);
        net_endpoint_buf_link(schedule->m_endpoint_tb);
    }
    else {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
        schedule->m_endpoint_tb = NULL;
        return 0;
    }
    
    schedule->m_endpoint_tb = NULL;

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_supply, size);
    }

    if (buf_type == net_ep_buf_write) {
        if (endpoint->m_driver->m_endpoint_on_output(endpoint) != 0) return -1;
    }

    if (buf_type == net_ep_buf_read) {
        if (endpoint->m_protocol->m_endpoint_input(endpoint) != 0) return -1;
    }

    return 0;
}

uint32_t net_endpoint_buf_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    if (endpoint->m_bufs[buf_type]) {
        return ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
    }
    else {
        return 0;
    }
}

void net_endpoint_buf_clear(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    if (endpoint->m_bufs[buf_type]) {
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
        endpoint->m_bufs[buf_type] = NULL;
    }
}

void net_endpoint_buf_consume(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);

    if (endpoint->m_bufs[buf_type]) {
        endpoint->m_bufs[buf_type] = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], size);

        if (endpoint->m_data_watcher_fun) {
            endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_consume, size);
        }
    }
}

void * net_endpoint_buf_peak(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t * size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    if (endpoint->m_bufs[buf_type] == NULL) return NULL;
    
    void * data;
    int block_size = ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, &data);

    assert(block_size >= 0);
    assert(data);
    
    *size = (uint32_t)block_size;
    return data;
}

int net_endpoint_buf_peak_with_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t require, void * * r_data) {
    assert(buf_type < net_ep_buf_count);

    if (endpoint->m_bufs[buf_type] == NULL) {
        *r_data = NULL;
        return 0;
    }

    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    void * data;
    int size = ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, &data);
    if (size >= require) {
        *r_data = data;
        return 0;
    }

    size = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
    if (size >= require) {
        ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)size);
        if (combine_blk == NULL) return -1;

        *r_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, combine_blk);
        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
        combine_blk->id = endpoint->m_id;
        endpoint->m_bufs[buf_type] = combine_blk;
        return 0;
    }
    else {
        *r_data = NULL;
        return 0;
    }
}

int net_endpoint_buf_recv(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void * data, uint32_t * size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t capacity = *size;
    uint32_t received = 0;

    while(capacity > 0 && endpoint->m_bufs[buf_type] != NULL) {
        char * block_data;
        uint32_t block_data_len = (uint32_t)ringbuffer_block_len(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, (void**)&block_data);

        uint32_t copy_len = block_data_len > capacity ? capacity : block_data_len;
        memcpy(data, block_data, copy_len);

        capacity -= copy_len;
        data += copy_len;
        received += copy_len;
        
        endpoint->m_bufs[buf_type] = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], (int)copy_len);
    }

    *size = received;

    if (received) {
        if (endpoint->m_data_watcher_fun) {
            endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_consume, received);
        }
    }
    
    return 0;
}

int net_endpoint_buf_by_sep(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * seps, void * * r_data, uint32_t *r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t sz = 0;
    ringbuffer_block_t block = endpoint->m_bufs[buf_type];
    while(block) {
        char * block_data;
        int block_data_len = ringbuffer_block_len(schedule->m_endpoint_buf, block, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, block, 0, (void**)&block_data);

        int block_pos;
        for(block_pos = 0; block_pos < block_data_len; ++block_pos) {
            int j;
            for(j = 0; seps[j] != 0; ++j) {
                if (block_data[block_pos] == seps[j]) {
                    if (block != endpoint->m_bufs[buf_type]) { /*多块，需要合并 */
                        block_pos += sz;
                        block_data_len = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
                        ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)block_data_len);
                        if (combine_blk == NULL) return -1;

                        block_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, combine_blk);
                        ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
                        combine_blk->id = endpoint->m_id;
                        endpoint->m_bufs[buf_type] = combine_blk;
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

static uint8_t net_endpoint_block_match_forward(
    net_schedule_t schedule, ringbuffer_block_t block, char * block_data, int block_data_len, int block_pos,
    const char * look_str, size_t look_str_len)
{
    int i;
    for(i = 0; i < 100; ++i) {
        assert(block_data_len >= block_pos);

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

int net_endpoint_buf_by_str(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * str, void * * r_data, uint32_t * r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    size_t str_len = strlen(str);
    if (str_len == 0) return -1;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t sz = 0;
    ringbuffer_block_t block = endpoint->m_bufs[buf_type];
    while(block) {
        char * block_data;
        int block_data_len = ringbuffer_block_len(schedule->m_endpoint_buf, block, 0);
        ringbuffer_block_data(schedule->m_endpoint_buf, block, 0, (void**)&block_data);

        int block_pos;
        for(block_pos = 0; block_pos < block_data_len; ++block_pos) {
            if (block_data[block_pos] == str[0]
                && net_endpoint_block_match_forward(
                    schedule, block, block_data, block_data_len, block_pos + 1, str + 1, str_len - 1)
                )
            {
                if (block != endpoint->m_bufs[buf_type] /*已经涉及多个块*/
                    || (block_pos + str_len) > block_data_len /**/)
                {
                    /*多块，需要合并 */
                    block_pos += sz;
                    block_data_len = ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
                    ringbuffer_block_t combine_blk = net_endpoint_common_buf_alloc(endpoint, (uint32_t)block_data_len);
                    if (combine_blk == NULL) return -1;

                    block_data = ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type], 0, combine_blk);
                    ringbuffer_free(schedule->m_endpoint_buf, endpoint->m_bufs[buf_type]);
                    combine_blk->id = endpoint->m_id;
                    endpoint->m_bufs[buf_type] = combine_blk;
                }

                block_data[block_pos] = 0;
                *r_data = block_data;
                *r_size = (uint32_t)(block_pos + str_len);
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

int net_endpoint_buf_append(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void const * i_data, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    if (size == 0) return 0;

    ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
    if (tb == NULL) return -1;

    void * data;
    ringbuffer_block_data(schedule->m_endpoint_buf, tb, 0, &data);
    assert(data);

    memcpy(data, i_data, size);

    ringbuffer_shrink(schedule->m_endpoint_buf, tb, (int)size);
    net_endpoint_buf_link(tb);

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_supply, size);
    }

    if (buf_type == net_ep_buf_write) {
        if (endpoint->m_driver->m_endpoint_on_output(endpoint) != 0) return -1;
    }

    if (buf_type == net_ep_buf_read) {
        if (endpoint->m_protocol->m_endpoint_input(endpoint) != 0) return -1;
    }

    return 0;
}

int net_endpoint_buf_append_char(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint8_t c) {
    return net_endpoint_buf_append(endpoint, buf_type, &c, sizeof(c));
}

int net_endpoint_buf_append_from_other(
    net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, net_endpoint_t other, net_endpoint_buf_type_t from, uint32_t size)
{
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    assert(from < net_ep_buf_count);

    if (size > 0) {
        uint32_t from_buf_len = other->m_bufs[from] ? ringbuffer_block_total_len(schedule->m_endpoint_buf, other->m_bufs[from]) : 0;
        if (size > from_buf_len) {
            CPE_ERROR(
                schedule->m_em, "core: %s: other.fbuf => wbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                from_buf_len, size);
            return -1;
        }

        ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
        if (tb == NULL) return -1;

        ringbuffer_copy(schedule->m_endpoint_buf, other->m_bufs[from], 0, tb);
        net_endpoint_buf_consume(other, from, size);

        net_endpoint_buf_link(tb);
    }
    else {
        if (other->m_bufs[from]) {
            net_endpoint_buf_link(other->m_bufs[from]);
            other->m_bufs[from] = NULL;
        }
    }

    if (other->m_data_watcher_fun) {
        other->m_data_watcher_fun(other->m_data_watcher_ctx, other, buf_type, net_endpoint_data_consume, size);
    }

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_supply, size);
    }

    if (buf_type == net_ep_buf_write) {
        if (endpoint->m_driver->m_endpoint_on_output(endpoint) != 0) return -1;
    }

    if (buf_type == net_ep_buf_read) {
        if (endpoint->m_protocol->m_endpoint_input(endpoint) != 0) return -1;
    }

    return 0;
}

int net_endpoint_buf_append_from_self(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, net_endpoint_buf_type_t from, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    assert(from < net_ep_buf_count);

    if (size > 0) {
        uint32_t from_buf_len = endpoint->m_bufs[from] ? ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_bufs[from]) : 0;
        if (size > from_buf_len) {
            CPE_ERROR(
                schedule->m_em, "core: %s: rbuf ==> fbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                from_buf_len, size);
            return -1;
        }
        
        ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
        if (tb == NULL) return -1;
        
        ringbuffer_copy(schedule->m_endpoint_buf, endpoint->m_bufs[from], 0, tb);

        net_endpoint_buf_link(tb);

        net_endpoint_buf_consume(endpoint, from, size);
    }
    else {
        if (endpoint->m_bufs[from]) {
            net_endpoint_buf_link(endpoint->m_bufs[from]);
            endpoint->m_bufs[from] = NULL;
        }
    }

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, from, net_endpoint_data_consume, size);
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_supply, size);
    }

    if (buf_type == net_ep_buf_write) {
        if (endpoint->m_driver->m_endpoint_on_output(endpoint) != 0) return -1;
    }

    if (buf_type == net_ep_buf_read) {
        if (endpoint->m_protocol->m_endpoint_input(endpoint) != 0) return -1;
    }

    return 0;
}
