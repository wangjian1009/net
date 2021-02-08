#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "net_mem_group_type_cache_i.h"
#include "net_mem_group_type_i.h"

net_mem_group_type_cache_group_t
net_mem_group_type_cache_find_group(net_mem_group_type_cache_t cache, uint32_t capacity) {
    uint8_t i;

    for(i = 0; i < CPE_ARRAY_SIZE(cache->m_groups); ++i) {
        net_mem_group_type_cache_group_t group = &cache->m_groups[i];
        if (capacity <= group->m_capacity) return group;
    }
    
    return NULL;
}

void * net_mem_group_type_cache_group_alloc(net_schedule_t schedule, net_mem_group_type_cache_group_t group) {
    net_mem_group_type_cache_block_t block = group->m_blocks;
    if (block) {
        assert(group->m_free_count > 0);

        group->m_blocks = block->m_next;
        group->m_free_count--;
        return block;
    }
    else {
        assert(group->m_free_count == 0);
        
        void * data = mem_alloc(schedule->m_alloc, group->m_capacity);
        if (data) group->m_alloc_count++;
        return data;
    }
}

void net_mem_group_type_cache_group_free(net_schedule_t schedule, net_mem_group_type_cache_group_t group, void * data) {
    assert(group->m_free_count < group->m_alloc_count);

    net_mem_group_type_cache_block_t block = data;
    block->m_next = group->m_blocks;
    group->m_blocks = block;
    group->m_free_count++;
}

int net_mem_group_type_cache_init(net_mem_group_type_t type) {
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);
    bzero(cache, sizeof(*cache));

    uint32_t capacity = 16;
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(cache->m_groups); ++i) {
        net_mem_group_type_cache_group_t group = &cache->m_groups[i];
        group->m_capacity = capacity;
        capacity *= 2;
    }
    
    return 0;
}

void net_mem_group_type_cache_fini(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(cache->m_groups); ++i) {
        net_mem_group_type_cache_group_t group = &cache->m_groups[i];
        assert(group->m_alloc_count == group->m_free_count);

        net_mem_group_type_cache_block_t block = group->m_blocks;
        while(block) {
            net_mem_group_type_cache_block_t to_free = block;
            block = block->m_next;
            mem_free(schedule->m_alloc, to_free);
        }
    }
}

void * net_mem_group_type_cache_alloc(net_mem_group_type_t type, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    net_mem_group_type_cache_group_t group = net_mem_group_type_cache_find_group(cache, capacity);

    return group
        ? net_mem_group_type_cache_group_alloc(schedule, group)
        : mem_alloc(type->m_schedule->m_alloc, capacity);
}

void net_mem_group_type_cache_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    net_mem_group_type_cache_group_t group = net_mem_group_type_cache_find_group(cache, capacity);

    group
        ? net_mem_group_type_cache_group_free(schedule, group, data)
        : mem_free(type->m_schedule->m_alloc, data);
}

net_mem_group_type_t
net_mem_group_type_cache_create(net_schedule_t schedule) {
    return net_mem_group_type_create(
        schedule, "cache",
        sizeof(struct net_mem_group_type_cache),
        net_mem_group_type_cache_init,
        net_mem_group_type_cache_fini,
        net_mem_group_type_cache_alloc, net_mem_group_type_cache_free);
}

