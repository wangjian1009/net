#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/math_ex.h"
#include "net_mem_group_type_cache_i.h"
#include "net_mem_group_type_i.h"

int net_mem_group_type_cache_group_add(
    net_mem_group_type_t type, net_mem_group_type_cache_t cache, uint32_t capacity)
{
    net_schedule_t schedule = type->m_schedule;

    if (cache->m_group_count >= CPE_ARRAY_SIZE(cache->m_groups)) {
        CPE_INFO(
            schedule->m_em, "net: core: mem gruop type: %s: init group %d",
            net_mem_type_str(type->m_type), capacity);
        return -1;
    }

    net_mem_group_type_cache_group_t group = &cache->m_groups[cache->m_group_count];
    
    group->m_capacity = capacity;
    group->m_alloc_count = 0;
    group->m_free_count = 0;
    group->m_blocks = NULL;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "net: core: mem gruop type: %s: %d ==> %d",
            net_mem_type_str(type->m_type), cache->m_group_count, capacity);
    }
    
    cache->m_group_count++;
    return 0;
}

net_mem_group_type_cache_group_t
net_mem_group_type_cache_find_group(net_mem_group_type_cache_t cache, uint32_t capacity) {
    uint8_t i;

    for(i = 0; i < cache->m_group_count; ++i) {
        net_mem_group_type_cache_group_t group = &cache->m_groups[i];
        if (capacity <= group->m_capacity) return group;
    }
    
    return NULL;
}

void * net_mem_group_type_cache_group_alloc(
    net_schedule_t schedule, net_mem_group_type_cache_t cache, net_mem_group_type_cache_group_t group)
{
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
        if (data) {
            cache->m_alloced_size += group->m_capacity;
            cache->m_alloced_count++;
            group->m_alloc_count++;
        }
        
        return data;
    }
}

void net_mem_group_type_cache_group_free(net_schedule_t schedule, net_mem_group_type_cache_group_t group, void * data) {
    assert(group->m_free_count <= group->m_alloc_count);

    net_mem_group_type_cache_block_t block = data;
    block->m_next = group->m_blocks;
    group->m_blocks = block;
    group->m_free_count++;
}

int net_mem_group_type_cache_init(net_mem_group_type_t type) {
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    cache->m_alloced_count = 0;
    cache->m_alloced_size = 0;
    cache->m_group_count = 0;
    
    uint32_t capacity = 16;
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(cache->m_groups); ++i) {
        if (net_mem_group_type_cache_group_add(type, cache, capacity) != 0) return -1;
        capacity *= 2;
    }

    TAILQ_INIT(&cache->m_big_blocks);
    
    return 0;
}

void net_mem_group_type_cache_fini(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    uint8_t i;
    for(i = 0; i < cache->m_group_count; ++i) {
        net_mem_group_type_cache_group_t group = &cache->m_groups[i];
        assert(group->m_alloc_count == group->m_free_count);

        net_mem_group_type_cache_block_t block = group->m_blocks;
        while(block) {
            assert(cache->m_alloced_count > 0);
            assert(cache->m_alloced_size >= group->m_capacity);
            
            net_mem_group_type_cache_block_t to_free = block;
            block = block->m_next;

            cache->m_alloced_count--;
            cache->m_alloced_size -= group->m_capacity;
            
            mem_free(schedule->m_alloc, to_free);
        }
    }

    while(!TAILQ_EMPTY(&cache->m_big_blocks)) {
        net_mem_group_type_cache_big_block_t  to_free = TAILQ_FIRST(&cache->m_big_blocks);

        TAILQ_REMOVE(&cache->m_big_blocks, to_free, m_next);
        
        cache->m_alloced_count--;
        cache->m_alloced_size -= to_free->m_capacity;
        mem_free(schedule->m_alloc, to_free);
    }

    assert(cache->m_alloced_count == 0);
    assert(cache->m_alloced_size == 0);
}

uint32_t net_mem_group_type_cache_suggest_size(net_mem_group_type_t type) {
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);
    if (cache->m_group_count > 1) {
        return cache->m_groups[cache->m_group_count - 2].m_capacity;
    }
    else if (cache->m_group_count > 0) {
        return cache->m_groups[cache->m_group_count - 1].m_capacity;
    }
    else {
        return 0;
    }
}

void * net_mem_group_type_cache_alloc(
    net_mem_group_type_t type, uint32_t * capacity, net_mem_alloc_capacity_policy_t policy)
{
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    net_mem_group_type_cache_group_t group = net_mem_group_type_cache_find_group(cache, *capacity);
    if (group == NULL && policy == net_mem_alloc_capacity_suggest) {
        group = &cache->m_groups[cache->m_group_count - 1];
    }

    if (group) {
        *capacity = group->m_capacity;
        return net_mem_group_type_cache_group_alloc(schedule, cache, group);
    }
    else {
        net_mem_group_type_cache_big_block_t block = NULL;
        TAILQ_FOREACH(block, &cache->m_big_blocks, m_next) {
            if (block->m_capacity >= *capacity) {
                TAILQ_REMOVE(&cache->m_big_blocks, block, m_next);
                *capacity = block->m_capacity;
                return block;
            }
        }
        
        uint32_t effect_capacity = cpe_math_32_round_to_pow2(*capacity);
        void * data =  mem_alloc(schedule->m_alloc, effect_capacity);
        if (data) {
            cache->m_alloced_count++;
            cache->m_alloced_size += effect_capacity;
            *capacity = effect_capacity;
        }

        return data;
    }
}

void net_mem_group_type_cache_free(net_mem_group_type_t type, void * data, uint32_t capacity) {
    net_schedule_t schedule = type->m_schedule;
    net_mem_group_type_cache_t cache = net_mem_group_type_data(type);

    net_mem_group_type_cache_group_t group = net_mem_group_type_cache_find_group(cache, capacity);

    if (group) {
        net_mem_group_type_cache_group_free(schedule, group, data);
    }
    else {
        net_mem_group_type_cache_big_block_t insert_after = NULL;

        net_mem_group_type_cache_big_block_t check_block = NULL;
        TAILQ_FOREACH(check_block, &cache->m_big_blocks, m_next) {
            if (check_block->m_capacity >= capacity) break;
            insert_after = check_block;
        }

        net_mem_group_type_cache_big_block_t free_block = data;
        free_block->m_capacity = capacity;

        if (insert_after) {
            TAILQ_INSERT_AFTER(&cache->m_big_blocks, insert_after, free_block, m_next);
        }
        else {
            TAILQ_INSERT_HEAD(&cache->m_big_blocks, free_block, m_next);
        }
    }
}

net_mem_group_type_t
net_mem_group_type_cache_create(net_schedule_t schedule) {
    return net_mem_group_type_create(
        schedule, net_mem_type_cache,
        sizeof(struct net_mem_group_type_cache),
        net_mem_group_type_cache_init, net_mem_group_type_cache_fini,
        net_mem_group_type_cache_suggest_size,
        net_mem_group_type_cache_alloc, net_mem_group_type_cache_free);
}

