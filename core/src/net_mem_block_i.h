#ifndef NET_MEM_BLOCK_I_H_INCLEDED
#define NET_MEM_BLOCK_I_H_INCLEDED
#include "net_mem_group_i.h"

struct net_mem_block {
    net_mem_group_t m_group;
    TAILQ_ENTRY(net_mem_block) m_next_for_group;
    net_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_mem_block) m_next_for_endpoint;
    net_endpoint_buf_type_t m_buf_type;
    uint32_t m_len;
    uint32_t m_capacity;
    uint8_t* m_data;
};

net_mem_block_t net_mem_block_create(net_mem_group_t group, uint32_t capacity);
void net_mem_block_free(net_mem_block_t mem_bloc);
void net_mem_block_real_free(net_mem_block_t mem_block);

void net_mem_block_erase(net_mem_block_t block, uint32_t size);
void net_mem_block_append(net_mem_block_t block, void const * data, uint32_t size);

void net_mem_block_link(net_mem_block_t block, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
void net_mem_block_unlink(net_mem_block_t block);
                             
#endif
