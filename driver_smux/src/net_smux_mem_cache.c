#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/math_ex.h"
#include "net_smux_mem_cache_i.h"

static net_smux_mem_group_t net_smux_mem_cache_find_group(net_smux_mem_cache_t cache, uint32_t capacity);
static net_smux_mem_block_t net_smux_mem_group_alloc(net_smux_mem_cache_t cache, net_smux_mem_group_t group);
static void net_smux_mem_group_release(net_smux_mem_group_t group, net_smux_mem_block_t block);

int net_smux_mem_group_add(net_smux_mem_cache_t cache, uint32_t capacity) {
    net_smux_protocol_t protocol = cache->m_protocol;

    if (cache->m_group_count >= CPE_ARRAY_SIZE(cache->m_groups)) {
        CPE_ERROR(protocol->m_em, "smux: init group %d", capacity);
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
net_smux_mem_cache_alloc(net_smux_mem_cache_t cache, uint16_t capacity) {
    net_smux_protocol_t protocol = cache->m_protocol;

    net_smux_mem_group_t group = net_smux_mem_cache_find_group(cache, capacity);

    if (group) {
        return net_smux_mem_group_alloc(cache, group);
    }
    else {
        /* net_smux_mem_cache_big_block_t block = NULL; */
        /* TAILQ_FOREACH(block, &cache->m_big_blocks, m_next) { */
        /*     if (block->m_capacity >= *capacity) { */
        /*         TAILQ_REMOVE(&cache->m_big_blocks, block, m_next); */

        /*         *capacity = block->m_capacity; */
        /*         return block; */
        /*     } */
        /* } */
        
        /* uint32_t effect_capacity = cpe_math_32_round_to_pow2(*capacity); */
        /* void * data =  mem_alloc(protocol->m_alloc, effect_capacity); */
        /* if (data) { */
        /*     cache->m_alloced_count++; */
        /*     cache->m_alloced_size += effect_capacity; */
        /*     *capacity = effect_capacity; */
        /* } */

        /* return data; */
        return NULL;
    }
}

void net_smux_mem_cache_release(net_smux_mem_cache_t cache, net_smux_mem_block_t block) {
    net_smux_protocol_t protocol = cache->m_protocol;

    net_smux_mem_group_t group = net_smux_mem_cache_find_group(cache, block->m_capacity);

    if (group) {
        net_smux_mem_group_release(group, block);
    }
    else {
        mem_free(protocol->m_alloc, block);
    }
}

net_smux_mem_cache_t
net_smux_mem_cache_create(net_smux_protocol_t protocol) {
    net_smux_mem_cache_t mem_cache = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_mem_cache));

    mem_cache->m_protocol = protocol;
    mem_cache->m_alloced_count = 0;
    mem_cache->m_alloced_size = 0;
    mem_cache->m_group_count = 0;
    
    uint32_t capacity = 16;
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(mem_cache->m_groups); ++i) {
        if (net_smux_mem_group_add(mem_cache, capacity) != 0) {
            return NULL;
        }
        
        capacity *= 2;
    }

    return 0;
}

void net_smux_mem_cache_fini(net_smux_mem_cache_t cache) {
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
net_smux_mem_group_alloc(net_smux_mem_cache_t cache, net_smux_mem_group_t group) {
    net_smux_protocol_t protocol = cache->m_protocol;

    net_smux_mem_block_t block = group->m_blocks;
    if (block) {
        assert(group->m_free_count > 0);

        group->m_blocks = block->m_next;
        group->m_free_count--;
        block->m_capacity = group->m_capacity;
        block->m_size = 0;
        return block;
    }
    else {
        assert(group->m_free_count == 0);

        void * data = mem_alloc(protocol->m_alloc, group->m_capacity);
        if (data) {
            cache->m_alloced_size += group->m_capacity;
            cache->m_alloced_count++;
            group->m_alloc_count++;
        }
        
        return data;
    }
}

void net_smux_mem_group_release(net_smux_mem_group_t group, net_smux_mem_block_t block) {
    assert(group->m_free_count < group->m_alloc_count);

    block->m_next = group->m_blocks;
    group->m_blocks = block;
    group->m_free_count++;
}

