#ifndef NET_MEM_GROUP_TYPE_CACHE_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_CACHE_I_H_INCLEDED
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_mem_group_type_cache * net_mem_group_type_cache_t;
typedef struct net_mem_group_type_cache_block * net_mem_group_type_cache_block_t;
typedef struct net_mem_group_type_cache_group * net_mem_group_type_cache_group_t;
typedef struct net_mem_group_type_cache_big_block * net_mem_group_type_cache_big_block_t;
typedef TAILQ_HEAD(net_mem_group_type_cache_big_block_list, net_mem_group_type_cache_big_block) net_mem_group_type_cache_big_block_list_t;

struct net_mem_group_type_cache_block {
    net_mem_group_type_cache_block_t m_next;
};

struct net_mem_group_type_cache_big_block {
    uint32_t m_capacity;
    TAILQ_ENTRY(net_mem_group_type_cache_big_block) m_next;
};

struct net_mem_group_type_cache_group {
    uint32_t m_capacity;
    uint32_t m_alloc_count;
    uint32_t m_free_count;
    net_mem_group_type_cache_block_t m_blocks;
};

struct net_mem_group_type_cache {
    uint32_t m_alloced_count;
    uint64_t m_alloced_size;
    uint8_t m_group_count;
    struct net_mem_group_type_cache_group m_groups[8];
    net_mem_group_type_cache_big_block_list_t m_big_blocks;
};

net_mem_group_type_t net_mem_group_type_cache_create(net_schedule_t schedule);

NET_END_DECL

#endif
