#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/math_ex.h"
#include "net_mem_block_i.h"
#include "net_endpoint_i.h"

net_mem_block_t net_mem_block_create(net_mem_group_t group, uint32_t capacity) {
    net_schedule_t schedule = group->m_schedule;
    
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

    block->m_endpoint = NULL;
    block->m_buf_type = 0;

    block->m_len = 0;
    block->m_capacity = capacity;
    block->m_data = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    if (block->m_data == NULL) {
        CPE_ERROR(schedule->m_em, "core: mem block: alloc buffer fail, capacity=%d!", capacity);
        block->m_group = (net_mem_group_t)schedule;
        TAILQ_INSERT_TAIL(&schedule->m_free_mem_blocks, block, m_next_for_group);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&group->m_blocks, block, m_next_for_group);
    
    return block;
}

net_mem_block_t net_mem_block_create_from(net_mem_group_t group, const uint8_t* data, uint32_t len) {
    net_mem_block_t result = net_mem_block_create(group, 2048);
    net_mem_block_store(result, data, len);
    return result;
}

void net_mem_block_free(net_mem_block_t block) {
    net_schedule_t schedule = block->m_group->m_schedule;
    
    assert(block);

    if (block->m_endpoint) {
        net_mem_block_unlink(block);
        assert(block->m_endpoint == NULL);
    }

    if (block->m_data != NULL) {
        free(block->m_data);
        block->m_data = NULL;
    }

    TAILQ_REMOVE(&block->m_group->m_blocks, block, m_next_for_group);
    
    block->m_group = (net_mem_group_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_mem_blocks, block, m_next_for_group);
}

void net_mem_block_real_free(net_mem_block_t block) {
    net_schedule_t schedule = (net_schedule_t)block->m_group;

    TAILQ_REMOVE(&schedule->m_free_mem_blocks, block, m_next_for_group);

    mem_free(schedule->m_alloc, block);
}

uint32_t net_mem_block_size(net_mem_block_t block) {
    return block ? block->m_len : 0;
}

uint8_t * net_mem_block_data(net_mem_block_t ptr) {
    return ptr ? ptr->m_data : NULL;
}

int net_mem_block_compare(net_mem_block_t ptr1, net_mem_block_t ptr2, uint32_t size) {
    if (ptr1 == NULL && ptr2 == NULL) {
        return 0;
    }
    if (ptr1 && ptr2 == NULL) {
        return -1;
    }
    if (ptr1 == NULL && ptr2) {
        return 1;
    }
    {
        uint32_t size1 = (size == UINT32_MAX) ? ptr1->m_len : cpe_min(size, ptr1->m_len);
        uint32_t size2 = (size == UINT32_MAX) ? ptr2->m_len : cpe_min(size, ptr2->m_len);
        uint32_t size0 = cpe_min(size1, size2);
        int ret = memcmp(ptr1->m_data, ptr2->m_data, size0);
        return (ret != 0) ? ret : ((size1 == size2) ? 0 : ((size0 == size1) ? 1 : -1));
    }
}

void net_mem_block_reset(net_mem_block_t ptr) {
    if (ptr && ptr->m_data) {
        ptr->m_len = 0;
        memset(ptr->m_data, 0, ptr->m_capacity);
    }
}

net_mem_block_t net_mem_block_clone(net_mem_group_t group, const net_mem_block_t block) {
    net_mem_block_t new_block = NULL;

    if (block == NULL) {
        return new_block;
    }

    new_block = net_mem_block_create(group, cpe_max(block->m_capacity, block->m_len));

    new_block->m_len = block->m_len;
    memcpy(new_block->m_data, block->m_data, block->m_len);
    
    return new_block;
}

uint32_t net_mem_block_realloc(net_mem_block_t block, uint32_t capacity) {
    uint32_t real_capacity = 0;
    if (block == NULL) {
        return real_capacity;
    }
    
    real_capacity = cpe_max(capacity, block->m_capacity);
    if (block->m_capacity < real_capacity) {
        block->m_data = (uint8_t*)realloc(block->m_data, real_capacity);
        assert(block->m_data);
        memset(block->m_data + block->m_capacity, 0, real_capacity - block->m_capacity);
        block->m_capacity = real_capacity;
    }
    
    return real_capacity;
}

uint32_t net_mem_block_store(net_mem_block_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = 0;
    if (ptr == NULL) {
        return result;
    }

    result = net_mem_block_realloc(ptr, size);
    if (ptr->m_data && data && size) {
        memcpy(ptr->m_data, data, size);
    }

    ptr->m_len = size;
    return cpe_min(size, result);
}

void net_mem_block_replace(net_mem_block_t dst, const net_mem_block_t src) {
    if (dst) {
        if (src) {
            net_mem_block_store(dst, src->m_data, src->m_len);
        } else {
            net_mem_block_reset(dst);
        }
    }
}

void net_mem_block_insert(net_mem_block_t ptr, uint32_t pos, const uint8_t* data, uint32_t size) {
    uint32_t result;
    if (ptr == NULL || data == NULL || size == 0) {
        return;
    }
    if (pos > ptr->m_len) {
        pos = ptr->m_len;
    }
    result = net_mem_block_realloc(ptr, ptr->m_len + size);
    memmove(ptr->m_data + pos + size, ptr->m_data + pos, ptr->m_len - pos);
    memmove(ptr->m_data + pos, data, size);
    ptr->m_len += size;
}

void net_mem_block_insert2(net_mem_block_t ptr, uint32_t pos, const net_mem_block_t data) {
    if (ptr == NULL || data == NULL) {
        return;
    }
    net_mem_block_insert(ptr, pos, data->m_data, data->m_len);
}

uint32_t net_mem_block_concatenate(net_mem_block_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = net_mem_block_realloc(ptr, ptr->m_len + size);
    memmove(ptr->m_data + ptr->m_len, data, size);
    ptr->m_len += size;
    return cpe_min(ptr->m_len, result);
}

uint32_t net_mem_block_concatenate2(net_mem_block_t dst, const net_mem_block_t src) {
    if (dst == NULL || src == NULL) { return 0; }
    return net_mem_block_concatenate(dst, src->m_data, src->m_len);
}

void net_mem_block_shortened_to(net_mem_block_t ptr, uint32_t begin, uint32_t len) {
    if (ptr && (begin <= ptr->m_len) && (len <= (ptr->m_len - begin))) {
        if (begin != 0) {
            memmove(ptr->m_data, ptr->m_data + begin, len);
        }
        ptr->m_data[len] = 0;
        ptr->m_len = len;
    }
}

void net_mem_block_erase(net_mem_block_t block, uint32_t size) {
    assert(block);
    assert(size <= block->m_len);

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
    net_endpoint_t endpoint = block->m_endpoint;
    if (endpoint) {
        assert(endpoint->m_bufs[block->m_buf_type].m_size >= size);

        endpoint->m_bufs[block->m_buf_type].m_size -= block->m_len;
        endpoint->m_bufs[block->m_buf_type].m_size += size;
    }
    
    assert(size <= block->m_capacity);
    block->m_len = size;
}

void net_mem_block_link(net_mem_block_t block, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type) {
    assert(block->m_endpoint == NULL);

    TAILQ_INSERT_TAIL(&endpoint->m_bufs[buf_type].m_blocks, block, m_next_for_endpoint);
    block->m_endpoint = endpoint;
    block->m_buf_type = buf_type;

    endpoint->m_bufs[buf_type].m_size += block->m_len;
}

void net_mem_block_unlink(net_mem_block_t block) {
    net_endpoint_t endpoint = block->m_endpoint;
    
    if (endpoint) {
        assert(endpoint->m_bufs[block->m_buf_type].m_size >= block->m_len);
        endpoint->m_bufs[block->m_buf_type].m_size -= block->m_len;

        TAILQ_REMOVE(&endpoint->m_bufs[block->m_buf_type].m_blocks, block, m_next_for_endpoint);
        block->m_endpoint = NULL;
    }
}
