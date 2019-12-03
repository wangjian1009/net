#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/math_ex.h"
#include "net_mem_block_i.h"

net_mem_block_t net_mem_block_create(net_schedule_t schedule, uint32_t capacity) {
    net_mem_block_t block = (net_mem_block_t)calloc(1, sizeof(struct net_mem_block));
    assert(block);
    block->m_schedule = schedule;
    block->m_endpoint = NULL;
    block->m_buf_type = 0;
    block->buffer = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    assert(block->buffer);
    block->capacity = capacity;
    block->ref_count = 1;
    return block;
}

void net_mem_block_add_ref(net_mem_block_t ptr) {
    if (ptr) {
        ptr->ref_count++;
    }
}

net_mem_block_t net_mem_block_create_from(net_schedule_t scheudle, const uint8_t* data, uint32_t len) {
    net_mem_block_t result = net_mem_block_create(scheudle, 2048);
    net_mem_block_store(result, data, len);
    return result;
}

uint32_t net_mem_block_get_length(const net_mem_block_t ptr) {
    return ptr ? ptr->len : 0;
}

const uint8_t* net_mem_block_get_data(const net_mem_block_t ptr, uint32_t* length) {
    if (length) {
        *length = net_mem_block_get_length(ptr);
    }
    return ptr ? ptr->buffer : NULL;
}

int net_mem_block_compare(const net_mem_block_t ptr1, const net_mem_block_t ptr2, uint32_t size) {
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
        uint32_t size1 = (size == SIZE_MAX) ? ptr1->len : cpe_min(size, ptr1->len);
        uint32_t size2 = (size == SIZE_MAX) ? ptr2->len : cpe_min(size, ptr2->len);
        uint32_t size0 = cpe_min(size1, size2);
        int ret = memcmp(ptr1->buffer, ptr2->buffer, size0);
        return (ret != 0) ? ret : ((size1 == size2) ? 0 : ((size0 == size1) ? 1 : -1));
    }
}

void net_mem_block_reset(net_mem_block_t ptr) {
    if (ptr && ptr->buffer) {
        ptr->len = 0;
        memset(ptr->buffer, 0, ptr->capacity);
    }
}

net_mem_block_t net_mem_block_clone(const net_mem_block_t ptr) {
    net_mem_block_t result = NULL;
    if (ptr == NULL) {
        return result;
    }
    result = net_mem_block_create(ptr->m_schedule, cpe_max(ptr->capacity, ptr->len));
    result->len = ptr->len;
    memmove(result->buffer, ptr->buffer, ptr->len);
    return result;
}

uint32_t net_mem_block_realloc(net_mem_block_t ptr, uint32_t capacity) {
    uint32_t real_capacity = 0;
    if (ptr == NULL) {
        return real_capacity;
    }
    real_capacity = cpe_max(capacity, ptr->capacity);
    if (ptr->capacity < real_capacity) {
        ptr->buffer = (uint8_t*)realloc(ptr->buffer, real_capacity);
        assert(ptr->buffer);
        memset(ptr->buffer + ptr->capacity, 0, real_capacity - ptr->capacity);
        ptr->capacity = real_capacity;
    }
    return real_capacity;
}

uint32_t net_mem_block_store(net_mem_block_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = 0;
    if (ptr == NULL) {
        return result;
    }
    result = net_mem_block_realloc(ptr, size);
    if (ptr->buffer && data && size) {
        memmove(ptr->buffer, data, size);
    }
    ptr->len = size;
    return cpe_min(size, result);
}

void net_mem_block_replace(net_mem_block_t dst, const net_mem_block_t src) {
    if (dst) {
        if (src) {
            net_mem_block_store(dst, src->buffer, src->len);
        } else {
            net_mem_block_reset(dst);
        }
    }
    /*
  if (dst==NULL || src==NULL) { return; }
  net_mem_block_store(dst, src->buffer, src->len);
  */
}

void net_mem_block_insert(net_mem_block_t ptr, uint32_t pos, const uint8_t* data, uint32_t size) {
    uint32_t result;
    if (ptr == NULL || data == NULL || size == 0) {
        return;
    }
    if (pos > ptr->len) {
        pos = ptr->len;
    }
    result = net_mem_block_realloc(ptr, ptr->len + size);
    memmove(ptr->buffer + pos + size, ptr->buffer + pos, ptr->len - pos);
    memmove(ptr->buffer + pos, data, size);
    ptr->len += size;
}

void net_mem_block_insert2(net_mem_block_t ptr, uint32_t pos, const net_mem_block_t data) {
    if (ptr == NULL || data == NULL) {
        return;
    }
    net_mem_block_insert(ptr, pos, data->buffer, data->len);
}

uint32_t net_mem_block_concatenate(net_mem_block_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = net_mem_block_realloc(ptr, ptr->len + size);
    memmove(ptr->buffer + ptr->len, data, size);
    ptr->len += size;
    return cpe_min(ptr->len, result);
}

uint32_t net_mem_block_concatenate2(net_mem_block_t dst, const net_mem_block_t src) {
    if (dst == NULL || src == NULL) { return 0; }
    return net_mem_block_concatenate(dst, src->buffer, src->len);
}

void net_mem_block_shortened_to(net_mem_block_t ptr, uint32_t begin, uint32_t len) {
    if (ptr && (begin <= ptr->len) && (len <= (ptr->len - begin))) {
        if (begin != 0) {
            memmove(ptr->buffer, ptr->buffer + begin, len);
        }
        ptr->buffer[len] = 0;
        ptr->len = len;
    }
}

void net_mem_block_release(net_mem_block_t ptr) {
    if (ptr == NULL) {
        return;
    }
    ptr->ref_count--;
    if (ptr->ref_count > 0) {
        return;
    }
    if (ptr->buffer != NULL) {
        free(ptr->buffer);
    }
    free(ptr);
}
