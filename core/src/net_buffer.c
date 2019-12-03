#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/math_ex.h"
#include "net_buffer_i.h"

static uint32_t _memory_size_internal(void* ptr) {
    if (ptr == NULL) {
        return 0;
    }
#if defined(_WIN32) && defined(_MSC_VER)
    return _msize(ptr);
#elif defined(__APPLE__)
    return malloc_size(ptr);
#else
    return malloc_usable_size(ptr); // Linux and __MINGW32__
#endif
}

net_buffer_t net_buffer_create(net_schedule_t scheudle, uint32_t capacity) {
    net_buffer_t ptr = (net_buffer_t)calloc(1, sizeof(struct net_buffer));
    assert(ptr);
    ptr->m_schedule = schedule;
    ptr->buffer = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    assert(ptr->buffer);
    ptr->capacity = capacity;
    ptr->ref_count = 1;
    assert(ptr->capacity <= _memory_size_internal(ptr->buffer));
    return ptr;
}

void net_buffer_add_ref(net_buffer_t ptr) {
    if (ptr) {
        ptr->ref_count++;
    }
}

net_buffer_t net_buffer_create_from(net_schedule_t scheudle, const uint8_t* data, uint32_t len) {
    net_buffer_t result = net_buffer_create(scheudle, 2048);
    net_buffer_store(result, data, len);
    return result;
}

uint32_t net_buffer_get_length(const net_buffer_t ptr) {
    return ptr ? ptr->len : 0;
}

const uint8_t* net_buffer_get_data(const net_buffer_t ptr, uint32_t* length) {
    if (length) {
        *length = net_buffer_get_length(ptr);
    }
    return ptr ? ptr->buffer : NULL;
}

int net_buffer_compare(const net_buffer_t ptr1, const net_buffer_t ptr2, uint32_t size) {
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

void net_buffer_reset(net_buffer_t ptr) {
    if (ptr && ptr->buffer) {
        ptr->len = 0;
        memset(ptr->buffer, 0, ptr->capacity);
    }
}

net_buffer_t net_buffer_clone(const net_buffer_t ptr) {
    net_buffer_t result = NULL;
    if (ptr == NULL) {
        return result;
    }
    result = net_buffer_create(cpe_max(ptr->capacity, ptr->len));
    result->len = ptr->len;
    memmove(result->buffer, ptr->buffer, ptr->len);
    return result;
}

uint32_t net_buffer_realloc(net_buffer_t ptr, uint32_t capacity) {
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
        assert(ptr->capacity <= _memory_size_internal(ptr->buffer));
    }
    return real_capacity;
}

uint32_t net_buffer_store(net_buffer_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = 0;
    if (ptr == NULL) {
        return result;
    }
    result = net_buffer_realloc(ptr, size);
    if (ptr->buffer && data && size) {
        memmove(ptr->buffer, data, size);
    }
    ptr->len = size;
    return cpe_min(size, result);
}

void net_buffer_replace(net_buffer_t dst, const net_buffer_t src) {
    if (dst) {
        if (src) {
            net_buffer_store(dst, src->buffer, src->len);
        } else {
            net_buffer_reset(dst);
        }
    }
    /*
  if (dst==NULL || src==NULL) { return; }
  net_buffer_store(dst, src->buffer, src->len);
  */
}

void net_buffer_insert(net_buffer_t ptr, uint32_t pos, const uint8_t* data, uint32_t size) {
    uint32_t result;
    if (ptr == NULL || data == NULL || size == 0) {
        return;
    }
    if (pos > ptr->len) {
        pos = ptr->len;
    }
    result = net_buffer_realloc(ptr, ptr->len + size);
    memmove(ptr->buffer + pos + size, ptr->buffer + pos, ptr->len - pos);
    memmove(ptr->buffer + pos, data, size);
    ptr->len += size;
}

void net_buffer_insert2(net_buffer_t ptr, uint32_t pos, const net_buffer_t data) {
    if (ptr == NULL || data == NULL) {
        return;
    }
    net_buffer_insert(ptr, pos, data->buffer, data->len);
}

uint32_t net_buffer_concatenate(net_buffer_t ptr, const uint8_t* data, uint32_t size) {
    uint32_t result = net_buffer_realloc(ptr, ptr->len + size);
    memmove(ptr->buffer + ptr->len, data, size);
    ptr->len += size;
    return cpe_min(ptr->len, result);
}

uint32_t net_buffer_concatenate2(net_buffer_t dst, const net_buffer_t src) {
    if (dst == NULL || src == NULL) { return 0; }
    return net_buffer_concatenate(dst, src->buffer, src->len);
}

void net_buffer_shortened_to(net_buffer_t ptr, uint32_t begin, uint32_t len) {
    if (ptr && (begin <= ptr->len) && (len <= (ptr->len - begin))) {
        if (begin != 0) {
            memmove(ptr->buffer, ptr->buffer + begin, len);
        }
        ptr->buffer[len] = 0;
        ptr->len = len;
    }
}

void net_buffer_release(net_buffer_t ptr) {
    if (ptr == NULL) {
        return;
    }
    ptr->ref_count--;
    if (ptr->ref_count > 0) {
        return;
    }
    if (ptr->buffer != NULL) {
        assert(ptr->capacity <= _memory_size_internal(ptr->buffer));
        free(ptr->buffer);
    }
    free(ptr);
}
