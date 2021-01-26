#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/math_ex.h"
#include "net_endpoint_i.h"
#include "net_mem_block_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"
#include "net_protocol_i.h"

#define net_endpoint_have_limit(__ep, __buf_type)                       \
    (((__ep)->m_bufs[(__buf_type)].m_limit != NET_ENDPOINT_NO_LIMIT)    \
     || ((__ep)->m_all_buf_limit != NET_ENDPOINT_NO_LIMIT)              \
     || ((__ep)->m_link && (__ep)->m_link->m_buf_limit != NET_ENDPOINT_NO_LIMIT))

static int net_endpoint_buf_on_supply(
    net_schedule_t schedule, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size);

net_mem_block_t net_endpoint_buf_combine(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
uint32_t net_endpoint_buf_calc_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

uint8_t net_endpoint_have_any_data(net_endpoint_t endpoint) {
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        if (!TAILQ_EMPTY(&endpoint->m_bufs[i].m_blocks)) return 1;
    }
    return 0;
}

uint32_t net_endpoint_all_buf_size(net_endpoint_t endpoint) {
    uint32_t size = 0;

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(endpoint->m_bufs); ++i) {
        size += endpoint->m_bufs[i].m_size;
    }

    return size;
}

uint8_t net_endpoint_buf_is_empty(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    assert(buf_type < net_ep_buf_count);
    return TAILQ_EMPTY(&endpoint->m_bufs[buf_type].m_blocks) ? 1 : 0;
}

net_mem_block_t net_endpoint_block_alloc(net_endpoint_t endpoint, uint32_t capacity) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(endpoint->m_tb == NULL);

    if (capacity == 0) {
        capacity = endpoint->m_dft_block_size ? endpoint->m_dft_block_size : 2048;
    }

    endpoint->m_tb = net_mem_block_create(endpoint->m_mem_group, capacity);
    if (endpoint->m_tb == NULL) {
        return NULL;
    }

    return endpoint->m_tb;
}

void * net_endpoint_buf_alloc(net_endpoint_t endpoint, uint32_t size) {
    return net_endpoint_buf_alloc_at_least(endpoint, &size);
}

void * net_endpoint_buf_alloc_at_least(net_endpoint_t endpoint, uint32_t * inout_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(inout_size);

    net_mem_block_t tb = net_endpoint_block_alloc(endpoint, *inout_size);
    if (tb == NULL) {
        return NULL;
    }
    
    *inout_size = tb->m_capacity;

    return tb->m_data;
}

net_mem_block_t net_endpoint_block_get(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    return TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
}

void net_endpoint_buf_release(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_tb) {
        net_mem_block_free(endpoint->m_tb);
        endpoint->m_tb = NULL;
    }
}

uint8_t net_endpoint_buf_validate(net_endpoint_t endpoint, void const * buf, uint32_t capacity) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_tb == NULL) return 0;

    assert(endpoint->m_tb);

    if (buf < (void *)endpoint->m_tb->m_data
        || ((const uint8_t *)buf) + capacity > endpoint->m_tb->m_data + endpoint->m_tb->m_capacity) return 0;

    return 1;
}

int net_endpoint_buf_supply(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);

    if (endpoint->m_tb == NULL) return 0;
    
    if (size > 0) {
        assert(size <= endpoint->m_tb->m_capacity);
        endpoint->m_tb->m_len = size;
        net_mem_block_link(endpoint->m_tb, endpoint, buf_type);
        endpoint->m_tb = NULL;
        return net_endpoint_buf_on_supply(schedule, endpoint, buf_type, size);
    }
    else {
        net_mem_block_free(endpoint->m_tb);
        endpoint->m_tb = NULL;
        return 0;
    }
}

uint32_t net_endpoint_buf_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);

    assert(net_endpoint_buf_calc_size(endpoint, buf_type) == endpoint->m_bufs[buf_type].m_size);

    return endpoint->m_bufs[buf_type].m_size;
}

void net_endpoint_buf_clear(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    while(!TAILQ_EMPTY(&endpoint->m_bufs[buf_type].m_blocks)) {
        net_mem_block_free(TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks));
    }
    assert(endpoint->m_bufs[buf_type].m_size == 0);
}

void net_endpoint_buf_consume(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);

    if (size == 0) return;
    
    assert(!TAILQ_EMPTY(&endpoint->m_bufs[buf_type].m_blocks));
    assert(endpoint->m_bufs[buf_type].m_size >= size);

    uint32_t left_size = size;
    while(left_size > 0) {
        net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
        assert(block);

        if (block->m_len <= left_size) {
            left_size -= block->m_len;
            net_mem_block_free(block);
        }
        else {
            net_mem_block_erase(block, left_size);
            left_size = 0;
        }
    }
    
    assert(endpoint->m_bufs[buf_type].m_size == net_endpoint_buf_calc_size(endpoint, buf_type));
    
    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_consume, size);
    }

    if (!net_endpoint_is_active(endpoint)) return;

    if (endpoint->m_close_after_send && !net_endpoint_have_any_data(endpoint)) {
        if (endpoint->m_protocol_debug || endpoint->m_driver_debug) {
            CPE_INFO(
                schedule->m_em, "core: %s: auto close on consume(close-after-send)!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        }

        if (net_endpoint_set_state(endpoint, net_endpoint_state_write_closed) != 0) {
            net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        }
        return;
    }
}

void * net_endpoint_buf_peak(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t * size) {
    assert(buf_type < net_ep_buf_count);

    net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
    if (block == NULL) {
        *size = 0;
        return NULL;
    }
    else {
        *size = block->m_len;
        return block->m_data;
    }
}

int net_endpoint_buf_peak_with_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t require, void * * r_data) {
    assert(buf_type < net_ep_buf_count);
    assert(require > 0);

    if (endpoint->m_bufs[buf_type].m_size < require) {
        *r_data = NULL;
        return 0;
    }

    net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
    assert(block);

    void * data;
    if (block->m_len >= require) {
        *r_data = block->m_data;
        return 0;
    }

    block = net_endpoint_buf_combine(endpoint, buf_type);
    if (block == NULL) {
        return -1;
    }

    *r_data = block->m_data;

    return 0;
}

int net_endpoint_buf_recv(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void * data, uint32_t * size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t capacity = *size;
    uint32_t received = 0;

    while(capacity > 0 && !TAILQ_EMPTY(&endpoint->m_bufs[buf_type].m_blocks)) {
        net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);

        assert(block->m_len <= endpoint->m_bufs[buf_type].m_size);

        uint32_t copy_len = cpe_min(block->m_len, capacity);
        memcpy(data, block->m_data, copy_len);

        capacity -= copy_len;
        data = ((char*)data) + copy_len;
        received += copy_len;

        if (copy_len == block->m_len) {
            net_mem_block_free(block);
        }
        else {
            net_mem_block_erase(block, copy_len);
        }
        assert(endpoint->m_bufs[buf_type].m_size == net_endpoint_buf_calc_size(endpoint, buf_type));
    }

    *size = received;

    if (received) {
        if (endpoint->m_data_watcher_fun) {
            endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_consume, received);
        }

        if (endpoint->m_close_after_send && net_endpoint_is_active(endpoint) && !net_endpoint_have_any_data(endpoint)) {
            if (endpoint->m_protocol_debug || endpoint->m_driver_debug) {
                CPE_INFO(
                    schedule->m_em, "core: %s: auto close on recv(close-after-send)!",
                    net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
            }

            if (net_endpoint_set_state(endpoint, net_endpoint_state_write_closed) != 0) {
                net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
            }

            return 0;
        }
    }
    
    return 0;
}

int net_endpoint_buf_by_sep(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * seps, void * * r_data, uint32_t *r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t sz = 0;
    net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
    while(block) {
        int block_pos;
        for(block_pos = 0; block_pos < block->m_len; ++block_pos) {
            int j;
            for(j = 0; seps[j] != 0; ++j) {
                if (block->m_data[block_pos] == seps[j]) {
                    if (block != TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks)) { /*多块，需要合并 */
                        block_pos += sz;
                        
                        block = net_endpoint_buf_combine(endpoint, buf_type);
                        if (block == NULL) {
                            return -1;
                        }
                    }

                    block->m_data[block_pos] = 0;
                    *r_data = block->m_data;
                    *r_size = block_pos + 1;
                    return 0;
                }
            }
        }
        sz += (uint32_t)block->m_len;
        block = TAILQ_NEXT(block, m_next_for_endpoint);
    }
    
    *r_data = NULL;
    *r_size = 0;
    return 0;
}

static uint8_t net_endpoint_block_match_forward(
    net_schedule_t schedule, net_mem_block_t block, uint32_t block_pos,
    const char * look_str, uint32_t look_str_len)
{
    int i;
    for(i = 0; i < 100; ++i) {
        assert(block->m_len >= block_pos);

        uint32_t cmp_sz = block->m_len - block_pos;
        if (cmp_sz > look_str_len) cmp_sz = look_str_len;

        if (memcmp(block->m_data + block_pos, look_str, cmp_sz) != 0) return 0;

        look_str += cmp_sz;
        look_str_len -= cmp_sz;

        if (look_str_len == 0) return 1;

        block = TAILQ_NEXT(block, m_next_for_endpoint);
        if (block == NULL) return 0;

        block_pos = 0;
    }

    assert(0);
    return 0;
}

int net_endpoint_buf_by_str(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * str, void * * r_data, uint32_t * r_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    uint32_t str_len = (uint32_t)strlen(str);
    if (str_len == 0) return -1;

    assert(buf_type < net_ep_buf_count);
    
    uint32_t sz = 0;
    net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
    while(block) {
        int block_pos;
        for(block_pos = 0; block_pos < block->m_len; ++block_pos) {
            if (block->m_data[block_pos] == str[0]
                && net_endpoint_block_match_forward(schedule, block, block_pos + 1, str + 1, str_len - 1))
            {
                if (block != TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks) /*已经涉及多个块*/
                    || (block_pos + str_len) > block->m_len /**/)
                {
                    /*多块，需要合并 */
                    block_pos += sz;

                    block = net_endpoint_buf_combine(endpoint, buf_type);
                    if (block == NULL) {
                        return -1;
                    }
                }

                block->m_data[block_pos] = 0;
                *r_data = block->m_data;
                *r_size = (uint32_t)(block_pos + str_len);
                return 0;
            }
        }
        sz += block->m_len;
        block = TAILQ_NEXT(block, m_next_for_endpoint);
    }
    
    *r_data = NULL;
    *r_size = 0;
    return 0;
}

int net_endpoint_buf_append(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void const * i_data, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    if (size == 0) return 0;

    if (endpoint->m_state == net_endpoint_state_deleting) return -1;

    net_mem_block_t last_block = TAILQ_LAST(&endpoint->m_bufs[buf_type].m_blocks, net_mem_block_list);
    if (last_block && (last_block->m_capacity - last_block->m_len) >= size) {
        net_mem_block_append(last_block, i_data, size);
    }
    else {
        net_mem_block_t new_block = net_mem_block_create(endpoint->m_mem_group, size);
        if (new_block == NULL) return -1;

        memcpy(new_block->m_data, i_data, size);
        new_block->m_len = size;

        net_mem_block_link(new_block, endpoint, buf_type);
    }
    
    return net_endpoint_buf_on_supply(schedule, endpoint, buf_type, size);
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

    assert(other->m_bufs[from].m_size == net_endpoint_buf_calc_size(other, from));

    uint32_t moved_size = 0;
    
    if (size > 0) {
        if (size > other->m_bufs[from].m_size) {
            CPE_ERROR(
                schedule->m_em, "core: %s: other.fbuf => wbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                other->m_bufs[from].m_size, size);
            return -1;
        }

        net_mem_block_t tb = net_mem_block_create(endpoint->m_mem_group, size);
        if (tb == NULL) return -1;

        while(!TAILQ_EMPTY(&other->m_bufs[from].m_blocks)) {
            net_mem_block_t from_block = TAILQ_FIRST(&other->m_bufs[from].m_blocks);

            uint32_t copy_size = cpe_min(size, from_block->m_len);
            net_mem_block_append(tb, from_block->m_data, copy_size);
            size -= copy_size;
            moved_size += copy_size;

            if (from_block->m_len > copy_size) {
                net_mem_block_erase(from_block, copy_size);
                assert(size == 0);
                break;
            }
            else {
                net_mem_block_free(from_block);
            }
        }
        assert(size == 0);

        net_mem_block_link(tb, endpoint, buf_type);
    }
    else {
        while(!TAILQ_EMPTY(&other->m_bufs[from].m_blocks)) {
            net_mem_block_t block = TAILQ_FIRST(&other->m_bufs[from].m_blocks);
            net_mem_block_unlink(block);
            net_mem_block_link(block, endpoint, buf_type);
            moved_size += block->m_len;
        }
    }

    if (moved_size == 0) return 0;

    /*处理other */
    if (other->m_data_watcher_fun) {
        other->m_data_watcher_fun(other->m_data_watcher_ctx, other, from, net_endpoint_data_consume, moved_size);
    }

    if (net_endpoint_is_active(other) && other->m_close_after_send && !net_endpoint_have_any_data(other)) {
        if (other->m_protocol_debug || other->m_driver_debug) {
            CPE_INFO(
                schedule->m_em, "core: %s: auto close on append_from_other(close-after-send)!",
                net_endpoint_dump(&schedule->m_tmp_buffer, other));
        }

        if (net_endpoint_set_state(other, net_endpoint_state_write_closed) != 0) {
            net_endpoint_set_state(other, net_endpoint_state_deleting);
        }
    }

    /*处理Self */
    return net_endpoint_buf_on_supply(schedule, endpoint, buf_type, moved_size);
}

int net_endpoint_buf_append_from_self(
    net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, net_endpoint_buf_type_t from, uint32_t size)
{
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(buf_type < net_ep_buf_count);
    assert(from < net_ep_buf_count);
    assert(buf_type != from);

    assert(endpoint->m_bufs[from].m_size == net_endpoint_buf_calc_size(endpoint, from));

    uint32_t moved_size = 0;
    if (size > 0) {
        if (size > endpoint->m_bufs[from].m_size) {
            CPE_ERROR(
                schedule->m_em, "core: %s: rbuf ==> fbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                endpoint->m_bufs[from].m_size, size);
            return -1;
        }
        
        net_mem_block_t tb = net_mem_block_create(endpoint->m_mem_group, size);
        if (tb == NULL) return -1;

        while(!TAILQ_EMPTY(&endpoint->m_bufs[from].m_blocks)) {
            net_mem_block_t from_block = TAILQ_FIRST(&endpoint->m_bufs[from].m_blocks);

            uint32_t copy_size = cpe_min(size, from_block->m_len);
            net_mem_block_append(tb, from_block->m_data, copy_size);
            size -= copy_size;
            moved_size += copy_size;

            if (from_block->m_len > copy_size) {
                net_mem_block_erase(from_block, copy_size);
                assert(size == 0);
                break;
            }
            else {
                net_mem_block_free(from_block);
            }
        }
        assert(size == 0);

        net_mem_block_link(tb, endpoint, buf_type);
    }
    else {
        while(!TAILQ_EMPTY(&endpoint->m_bufs[from].m_blocks)) {
            net_mem_block_t block = TAILQ_FIRST(&endpoint->m_bufs[from].m_blocks);
            net_mem_block_unlink(block);
            net_mem_block_link(block, endpoint, buf_type);
            moved_size += block->m_len;
        }

        if (moved_size == 0) return 0;
    }

    if (endpoint->m_close_after_send && net_endpoint_is_active(endpoint) && !net_endpoint_have_any_data(endpoint)) {
        if (endpoint->m_protocol_debug || endpoint->m_driver_debug) {
            CPE_INFO(
                schedule->m_em, "core: %s: auto close on recv(close-after-send)!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        }

        if (net_endpoint_set_state(endpoint, net_endpoint_state_write_closed) != 0) {
            net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
        }

        return 0;
    }
    
    return net_endpoint_buf_on_supply(schedule, endpoint, buf_type, moved_size);
}

static int net_endpoint_buf_on_supply(net_schedule_t schedule, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size) {
    assert(size > 0);
    
    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, buf_type, net_endpoint_data_supply, size);
    }

    if (buf_type == net_ep_buf_read) {
        if (endpoint->m_protocol->m_endpoint_input(endpoint) != 0) return -1;
    }

    if (buf_type == net_ep_buf_write) {
        if (endpoint->m_state == net_endpoint_state_established) {
            if (endpoint->m_driver->m_endpoint_update(endpoint) != 0) return -1;
        }
    }

    if (buf_type == net_ep_buf_write && endpoint->m_state == net_endpoint_state_established) {
        if (endpoint->m_driver->m_endpoint_update(endpoint) != 0) return -1;
    }
    
    return 0;
}

net_mem_block_t net_endpoint_buf_combine(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    if (endpoint->m_bufs[buf_type].m_size == 0) return NULL;
    
    net_mem_block_t block = net_mem_block_create(endpoint->m_mem_group, endpoint->m_bufs[buf_type].m_size);
    if (block == NULL) {
        return NULL;
    }

    while(!TAILQ_EMPTY(&endpoint->m_bufs[buf_type].m_blocks)) {
        net_mem_block_t old_block = TAILQ_FIRST(&endpoint->m_bufs[buf_type].m_blocks);
        memcpy(block->m_data + block->m_len, old_block->m_data, old_block->m_len);
        block->m_len += old_block->m_len;
        net_mem_block_free(old_block);
    }

    net_mem_block_link(block, endpoint, buf_type);

    return block;
}

uint32_t net_endpoint_buf_calc_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    uint32_t size = 0;

    net_mem_block_t block;
    TAILQ_FOREACH(block, &endpoint->m_bufs[buf_type].m_blocks, m_next_for_endpoint) {
        size += block->m_len;
    }
    
    return size;
}
