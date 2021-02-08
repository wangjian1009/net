#ifndef NET_MEM_GROUP_TYPE_CACHE_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_CACHE_I_H_INCLEDED
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_mem_group_type_cache * net_mem_group_type_cache_t;
typedef struct net_mem_group_type_cache_block * net_mem_group_type_cache_block_t;
typedef struct net_mem_group_type_cache_group * net_mem_group_type_cache_group_t;

struct net_mem_group_type_cache_block {
    net_mem_group_type_cache_block_t m_next;
};

struct net_mem_group_type_cache_group {
    uint32_t m_capacity;
    uint16_t m_alloc_count;
    uint16_t m_free_count;
    net_mem_group_type_cache_block_t m_blocks;
};

struct net_mem_group_type_cache {
    struct net_mem_group_type_cache_group m_groups[8];
};

net_mem_group_type_t net_mem_group_type_cache_create(net_schedule_t schedule);

NET_END_DECL

#endif
