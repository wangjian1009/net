#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/ringbuffer.h"
#include "net_endpoint_i.h"
#include "net_driver_i.h"
#include "net_schedule_i.h"

#define net_endpoint_wbuf_link(__buf)                                   \
    do {                                                                \
        if (endpoint->m_wb) {                                           \
            ringbuffer_link(                                            \
                schedule->m_endpoint_buf, endpoint->m_wb, (__buf));     \
            assert(endpoint->m_wb->id == endpoint->m_id);               \
        }                                                               \
        else {                                                          \
            ringbuffer_block_set_id(                                    \
                schedule->m_endpoint_buf, (__buf), endpoint->m_id);     \
            endpoint->m_wb = (__buf);                                   \
        }                                                               \
    } while(0)

uint8_t net_endpoint_wbuf_is_full(net_endpoint_t endpoint) {
    return 0;
}

uint8_t net_endpoint_wbuf_is_empty(net_endpoint_t endpoint) {
    return endpoint->m_wb ? 0 : 1;
}

uint32_t net_endpoint_wbuf_size(net_endpoint_t endpoint) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (endpoint->m_wb) {
		return ringbuffer_block_total_len(schedule->m_endpoint_buf, endpoint->m_wb);
	}
    else {
        return 0;
	}
}

void * net_endpoint_wbuf_alloc(net_endpoint_t endpoint, uint32_t * inout_size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    void * data;
    int size;

    if (schedule->m_endpoint_tb) {
        size = ringbuffer_block_data(schedule->m_endpoint_buf, schedule->m_endpoint_tb, 0, &data);
        if (size > 0 && (*inout_size == 0 ||  (uint32_t)size > *inout_size)) {
            *inout_size = (uint32_t)size;
            assert(data);
            return data;
        }
        else {
            ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
            schedule->m_endpoint_tb = NULL;
        }
    }

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

int net_endpoint_wbuf_supply(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    assert(schedule->m_endpoint_tb);

    if (size) {
        ringbuffer_shrink(schedule->m_endpoint_buf, schedule->m_endpoint_tb, (int)size);
        net_endpoint_wbuf_link(schedule->m_endpoint_tb);
    }
    else {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
    }
    
    schedule->m_endpoint_tb = NULL;

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, net_endpoint_data_w_supply, size);
    }
    
    return endpoint->m_driver->m_endpoint_on_output(endpoint);
}

void * net_endpoint_wbuf(net_endpoint_t endpoint, uint32_t * size) {
    if (endpoint->m_wb == NULL) return NULL;

    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    void * data;
    int block_size = ringbuffer_block_data(schedule->m_endpoint_buf, endpoint->m_wb, 0, &data);

    assert(block_size >= 0);
    assert(data);
    
    *size = (uint32_t)block_size;
    return data;
}

void net_endpoint_wbuf_consume(net_endpoint_t endpoint, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (schedule->m_endpoint_tb) {
        ringbuffer_free(schedule->m_endpoint_buf, schedule->m_endpoint_tb);
        schedule->m_endpoint_tb = NULL;
    }
    
    endpoint->m_wb = ringbuffer_yield(schedule->m_endpoint_buf, endpoint->m_wb, size);
    assert(endpoint->m_wb == NULL || endpoint->m_wb->id == endpoint->m_id);

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, net_endpoint_data_w_consume, size);
    }

    if (endpoint->m_close_after_send) {
        if (endpoint->m_protocol_debug || endpoint->m_driver_debug) {
            CPE_INFO(
                schedule->m_em, "core: %s: auto close aftern all data sended!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint));
        }
        
        net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
    }
}

int net_endpoint_wbuf_append(net_endpoint_t endpoint, void const * i_data, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (size == 0) return 0;

    ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
    if (tb == NULL) return -1;

    void * data;
    ringbuffer_block_data(schedule->m_endpoint_buf, tb, 0, &data);
    assert(data);

    memcpy(data, i_data, size);

    ringbuffer_shrink(schedule->m_endpoint_buf, tb, (int)size);
    net_endpoint_wbuf_link(tb);

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, net_endpoint_data_w_supply, size);
    }
    
    return endpoint->m_driver->m_endpoint_on_output(endpoint);
}

int net_endpoint_wbuf_append_from_other(net_endpoint_t endpoint, net_endpoint_t other, uint32_t size) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (size > 0) {
        uint32_t fbuf_len = other->m_fb ? ringbuffer_block_total_len(schedule->m_endpoint_buf, other->m_fb) : 0;
        if (size > fbuf_len) {
            CPE_ERROR(
                schedule->m_em, "core: %s: other.fbuf => wbuf, only %d data, but move %d!",
                net_endpoint_dump(&schedule->m_tmp_buffer, endpoint),
                fbuf_len, size);
            return -1;
        }

        ringbuffer_block_t tb = net_endpoint_common_buf_alloc(endpoint, size);
        if (tb == NULL) return -1;

        ringbuffer_copy(schedule->m_endpoint_buf, other->m_fb, 0, tb);
        net_endpoint_fbuf_consume(other, size);
        assert(other->m_fb == NULL);

        net_endpoint_wbuf_link(tb);
    }
    else {
        if (other->m_fb) {
            net_endpoint_wbuf_link(other->m_fb);
            other->m_fb = NULL;
        }
    }

    if (other->m_data_watcher_fun) {
        other->m_data_watcher_fun(other->m_data_watcher_ctx, other, net_endpoint_data_f_consume, size);
    }
    

    if (endpoint->m_data_watcher_fun) {
        endpoint->m_data_watcher_fun(endpoint->m_data_watcher_ctx, endpoint, net_endpoint_data_w_supply, size);
    }
    
    return endpoint->m_driver->m_endpoint_on_output(endpoint);
}
