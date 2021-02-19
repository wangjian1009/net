#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/math_ex.h"
#include "net_smux_mem_cache_i.h"

static net_smux_mem_group_t net_smux_mem_cache_find_group(net_smux_mem_cache_t cache, uint32_t capacity);
static net_smux_mem_block_t net_smux_mem_group_alloc(net_smux_mem_cache_t cache, net_smux_mem_group_t group, uint16_t len);
static void net_smux_mem_group_release(net_smux_mem_group_t group, net_smux_mem_block_t block);

int net_smux_mem_group_add(net_smux_mem_cache_t cache, uint32_t capacity) {
    net_smux_protocol_t protocol = cache->m_protocol;

    if (cache->m_group_count >= CPE_ARRAY_SIZE(cache->m_groups)) {
        CPE_ERROR(protocol->m_em, "smux: mem-cache: create: create group fail, capacity=%d!", capacity);
        return -1;
    }

    net_smux_mem_group_t group = &cache->m_groups[cache->m_group_count];
    
    group->m_capacity = capacity;
    group->m_alloc_count = 0;
    group->m_free_count = 0;
    group->m_blocks = NULL;

    cache->m_group_count++;
    return 0;
}

net_smux_mem_block_t
net_smux_mem_cache_alloc(net_smux_mem_cache_t cache, uint16_t len) {
    net_smux_protocol_t protocol = cache->m_protocol;

    uint32_t capacity = len + sizeof(struct net_smux_mem_block);
    net_smux_mem_group_t group = net_smux_mem_cache_find_group(cache, capacity);

    if (group == NULL) {
        CPE_ERROR(protocol->m_em, "smux: mem-cache: alloc: find group fail, len=%d, capacity=%d!", len, capacity);
        return NULL;
    }
    
    return net_smux_mem_group_alloc(cache, group, len);
}

void net_smux_mem_cache_release(net_smux_mem_cache_t cache, net_smux_mem_block_t block) {
    net_smux_protocol_t protocol = cache->m_protocol;

    uint32_t capacity = block->m_capacity + sizeof(struct net_smux_mem_block);
    net_smux_mem_group_t group = net_smux_mem_cache_find_group(cache, capacity);

    if (group) {
        net_smux_mem_group_release(group, block);
    }
    else {
        assert(0);
    }
}

net_smux_mem_cache_t
net_smux_mem_cache_create(net_smux_protocol_t protocol, net_smux_config_t config) {
    net_smux_mem_cache_t cache = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_mem_cache));
    if (cache == NULL) {
        CPE_ERROR(protocol->m_em, "smux: mem-cache: create: alloc fail!");
        return NULL;
    }

    cache->m_protocol = protocol;
    cache->m_alloced_count = 0;
    cache->m_alloced_size = 0;
    cache->m_group_count = 0;

    uint32_t max_frame_size_with_head = config->m_max_frame_size + sizeof(struct net_smux_mem_block);
    uint32_t capacity = 32;
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(cache->m_groups); ++i) {
        if (capacity > max_frame_size_with_head) {
            capacity = max_frame_size_with_head;
        }
        
        if (net_smux_mem_group_add(cache, capacity) != 0) return NULL;

        if (capacity == max_frame_size_with_head) break;

        capacity *= 2;
    }

    if (capacity < max_frame_size_with_head) {
        CPE_ERROR(
            protocol->m_em, "smux: mem-cache: create: not reach max-frame-size=%d, capacity=%d!",
            config->m_max_frame_size, capacity);
        net_smux_mem_cache_free(cache);
        return NULL;
    }
    
    return cache;
}

void net_smux_mem_cache_free(net_smux_mem_cache_t cache) {
    net_smux_protocol_t protocol = cache->m_protocol;

    uint8_t i;
    for(i = 0; i < cache->m_group_count; ++i) {
        net_smux_mem_group_t group = &cache->m_groups[i];
        assert(group->m_alloc_count == group->m_free_count);

        net_smux_mem_block_t block = group->m_blocks;
        while(block) {
            assert(cache->m_alloced_count > 0);
            assert(cache->m_alloced_size >= group->m_capacity);
            
            net_smux_mem_block_t to_free = block;
            block = block->m_next;

            cache->m_alloced_count--;
            cache->m_alloced_size -= group->m_capacity;
            
            mem_free(protocol->m_alloc, to_free);
        }
    }

    assert(cache->m_alloced_count == 0);
    assert(cache->m_alloced_size == 0);
}

net_smux_mem_group_t
net_smux_mem_cache_find_group(net_smux_mem_cache_t cache, uint32_t capacity) {
    uint8_t i;

    for(i = 0; i < cache->m_group_count; ++i) {
        net_smux_mem_group_t group = &cache->m_groups[i];
        if (capacity <= group->m_capacity) return group;
    }
    
    return NULL;
}

net_smux_mem_block_t
net_smux_mem_group_alloc(net_smux_mem_cache_t cache, net_smux_mem_group_t group, uint16_t len) {
    net_smux_protocol_t protocol = cache->m_protocol;

    assert(sizeof(struct net_smux_mem_block) + len <= group->m_capacity);

    net_smux_mem_block_t block = group->m_blocks;
    if (block) {
        assert(group->m_free_count > 0);

        group->m_blocks = block->m_next;
        group->m_free_count--;
    }
    else {
        assert(group->m_free_count == 0);

        void * data = mem_alloc(protocol->m_alloc, group->m_capacity);
        if (data == NULL) {
            CPE_ERROR(protocol->m_em, "smux: mem-cache: create: alloc block, capacity=%d!", group->m_capacity);
            return NULL;
        }

        cache->m_alloced_size += group->m_capacity;
        cache->m_alloced_count++;
        group->m_alloc_count++;
    }

    block->m_capacity = group->m_capacity - sizeof(struct net_smux_mem_block);
    block->m_size = len;
    return block;
}

void net_smux_mem_group_release(net_smux_mem_group_t group, net_smux_mem_block_t block) {
    assert(group->m_free_count < group->m_alloc_count);

    block->m_next = group->m_blocks;
    group->m_blocks = block;
    group->m_free_count++;
}

