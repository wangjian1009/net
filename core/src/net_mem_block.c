#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/math_ex.h"
#include "net_mem_block_i.h"
#include "net_endpoint_i.h"
#include "net_progress_i.h"

void net_mem_block_set_size(net_mem_block_t block, uint32_t size);

net_mem_block_t
net_mem_block_create(net_mem_group_t group, uint32_t ep_id, uint32_t capacity, net_mem_alloc_capacity_policy_t policy) {
    net_schedule_t schedule = group->m_type->m_schedule;
    
    net_mem_block_t block = TAILQ_FIRST(&schedule->m_free_mem_blocks);
    if (block) {
        TAILQ_REMOVE(&schedule->m_free_mem_blocks, block, m_next_for_group);
    }
    else {
        block = mem_alloc(schedule->m_alloc, sizeof(struct net_mem_block));
        if (block == NULL) {
            CPE_ERROR(schedule->m_em, "core: mem block: alloc block fail!");
            return NULL;
        }
    }
    
    block->m_group = group;

    block->m_ep_id = ep_id;
    block->m_bind_type = net_mem_block_bind_none;

    block->m_len = 0;
    block->m_capacity = capacity;
    block->m_data = group->m_type->m_block_alloc(group->m_type, ep_id, &block->m_capacity, policy);
    if (block->m_data == NULL) {
        CPE_ERROR(schedule->m_em, "core: mem block: alloc buffer fail, capacity=%d!", block->m_capacity);
        block->m_group = (net_mem_group_t)schedule;
        TAILQ_INSERT_TAIL(&schedule->m_free_mem_blocks, block, m_next_for_group);
        return NULL;
    }

    group->m_alloced_count++;
    group->m_alloced_size += block->m_capacity;
    TAILQ_INSERT_TAIL(&group->m_blocks, block, m_next_for_group);

    return block;
}

void net_mem_block_clear_bind(net_mem_block_t block) {
}

void net_mem_block_free(net_mem_block_t block) {
    net_mem_group_t group = block->m_group;
    net_mem_group_type_t type = group->m_type;
    net_schedule_t schedule = type->m_schedule;
    
    assert(block);

    net_mem_block_unlink(block);
    
    if (block->m_data != NULL) {
        type->m_block_free(type, block->m_data, block->m_capacity);
        block->m_data = NULL;
    }

    assert(group->m_alloced_count > 0);
    group->m_alloced_count--;
    assert(group->m_alloced_size >= block->m_capacity);
    group->m_alloced_size -= block->m_capacity;
    
    TAILQ_REMOVE(&block->m_group->m_blocks, block, m_next_for_group);
    
    block->m_group = (net_mem_group_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_mem_blocks, block, m_next_for_group);
}

void net_mem_block_real_free(net_mem_block_t block) {
    net_schedule_t schedule = (net_schedule_t)block->m_group;

    TAILQ_REMOVE(&schedule->m_free_mem_blocks, block, m_next_for_group);

    mem_free(schedule->m_alloc, block);
}

void net_mem_block_reset(net_mem_block_t ptr) {
    if (ptr && ptr->m_data) {
        ptr->m_len = 0;
        memset(ptr->m_data, 0, ptr->m_capacity);
    }
}

void net_mem_block_erase(net_mem_block_t block, uint32_t size) {
    assert(block);
    assert(size < block->m_len); /*如果没有剩余数据，因该free */

    memmove(block->m_data, block->m_data + size, block->m_len - size);
    net_mem_block_set_size(block, block->m_len - size);
}

void net_mem_block_append(net_mem_block_t block, void const * data, uint32_t size) {
    assert(block);
    assert(block->m_capacity - block->m_len >= size);

    memcpy(block->m_data + block->m_len, data, size);
    net_mem_block_set_size(block, block->m_len + size);
}

void net_mem_block_set_size(net_mem_block_t block, uint32_t size) {
    switch(block->m_bind_type) {
    case net_mem_block_bind_none:
        break;
    case net_mem_block_bind_endpoint: {
        net_endpoint_t endpoint = block->m_endpoint.m_endpoint;
        if (endpoint) {
            assert(endpoint->m_bufs[block->m_endpoint.m_buf_type].m_size >= block->m_len);

            endpoint->m_bufs[block->m_endpoint.m_buf_type].m_size -= block->m_len;
            endpoint->m_bufs[block->m_endpoint.m_buf_type].m_size += size;
        }
        break;
    }
    case net_mem_block_bind_progress: {
        net_progress_t progress = block->m_progress.m_progress;
        if (progress) {
            assert(progress->m_size >= block->m_len);

            progress->m_size -= block->m_len;
            progress->m_size += size;
        }
        break;
    }
    }

    assert(size <= block->m_capacity);
    block->m_len = size;
}

void net_mem_block_link_endpoint(net_mem_block_t block, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    net_mem_block_unlink(block);
    assert(block->m_bind_type == net_mem_block_bind_none);

    if (block->m_ep_id != endpoint->m_id) {
        if (endpoint->m_mem_group->m_type->m_block_update_ep) {
            endpoint->m_mem_group->m_type->m_block_update_ep(
                endpoint->m_mem_group->m_type, endpoint->m_id, block->m_data, block->m_capacity);
        }
        block->m_ep_id = endpoint->m_id;
    }

    TAILQ_INSERT_TAIL(&endpoint->m_bufs[buf_type].m_blocks, block, m_endpoint.m_next);
    block->m_endpoint.m_endpoint = endpoint;
    block->m_endpoint.m_buf_type = buf_type;

    endpoint->m_bufs[buf_type].m_size += block->m_len;

    block->m_bind_type = net_mem_block_bind_endpoint;
}

void net_mem_block_link_progress(net_mem_block_t block, net_progress_t progress) {
    net_mem_block_unlink(block);
    assert(block->m_bind_type == net_mem_block_bind_none);

    if (block->m_ep_id != progress->m_id) {
        if (progress->m_mem_group->m_type->m_block_update_ep) {
            progress->m_mem_group->m_type->m_block_update_ep(
                progress->m_mem_group->m_type, progress->m_id, block->m_data, block->m_capacity);
        }
        block->m_ep_id = progress->m_id;
    }

    TAILQ_INSERT_TAIL(&progress->m_blocks, block, m_progress.m_next);
    block->m_progress.m_progress = progress;
    progress->m_size += block->m_len;

    block->m_bind_type = net_mem_block_bind_progress;
}

void net_mem_block_unlink(net_mem_block_t block) {
    switch(block->m_bind_type) {
    case net_mem_block_bind_none:
        break;
    case net_mem_block_bind_endpoint:
        if (block->m_endpoint.m_endpoint) {
            net_endpoint_t endpoint = block->m_endpoint.m_endpoint;
            assert(endpoint->m_bufs[block->m_endpoint.m_buf_type].m_size >= block->m_len);
            endpoint->m_bufs[block->m_endpoint.m_buf_type].m_size -= block->m_len;

            TAILQ_REMOVE(&endpoint->m_bufs[block->m_endpoint.m_buf_type].m_blocks, block, m_endpoint.m_next);
            block->m_endpoint.m_endpoint = NULL;
        }
        block->m_bind_type = net_mem_block_bind_none;
        break;
    case net_mem_block_bind_progress:
        if (block->m_progress.m_progress) {
            net_progress_t progress = block->m_progress.m_progress;
            assert(progress->m_size >= block->m_len);
            progress->m_size -= block->m_len;

            TAILQ_REMOVE(&progress->m_blocks, block, m_endpoint.m_next);
            block->m_progress.m_progress = NULL;
        }
        block->m_bind_type = net_mem_block_bind_none;
        break;
    }
}
