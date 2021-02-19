#ifndef NET_SMUX_MEM_GROUP_I_H_INCLEDED
#define NET_SMUX_MEM_GROUP_I_H_INCLEDED
#include "net_smux_protocol_i.h"

NET_BEGIN_DECL

struct net_smux_mem_block {
    union {
        net_smux_mem_block_t m_next;
        struct {
            uint16_t m_capacity;
            uint16_t m_size;
        };
    };
};

struct net_smux_mem_group {
    uint32_t m_capacity;
    uint16_t m_alloc_count;
    uint16_t m_free_count;
    net_smux_mem_block_t m_blocks;
};

struct net_smux_mem_cache {
    net_smux_protocol_t m_protocol;
    uint16_t m_alloced_count;
    uint32_t m_alloced_size;
    uint8_t m_group_count;
    struct net_smux_mem_group m_groups[12];
};

net_smux_mem_cache_t
net_smux_mem_cache_create(net_smux_protocol_t protocol, net_smux_config_t config);

void net_smux_mem_cache_free(net_smux_mem_cache_t cache);

void net_smux_mem_cache_release(net_smux_mem_cache_t cache, net_smux_mem_block_t block);
net_smux_mem_block_t net_smux_mem_cache_alloc(net_smux_mem_cache_t cache, uint16_t capacity);
    
NET_END_DECL

#endif
