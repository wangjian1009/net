#ifndef NET_MEM_BLOCK_I_H_INCLEDED
#define NET_MEM_BLOCK_I_H_INCLEDED
#include "net_mem_group_type.h"
#include "net_mem_group_i.h"

enum net_mem_block_bind_type {
    net_mem_block_bind_none,
    net_mem_block_bind_endpoint,
    net_mem_block_bind_progress,
};

struct net_mem_block {
    net_mem_group_t m_group;
    TAILQ_ENTRY(net_mem_block) m_next_for_group;
    uint32_t m_ep_id;
    enum net_mem_block_bind_type m_bind_type;
    union {
        struct {
            net_endpoint_t m_endpoint;
            TAILQ_ENTRY(net_mem_block) m_next;
            net_endpoint_buf_type_t m_buf_type;
        } m_endpoint;
        struct {
            net_progress_t m_progress;
            TAILQ_ENTRY(net_mem_block) m_next;
        } m_progress;
    };
    uint32_t m_len;
    uint32_t m_capacity;
    uint8_t* m_data;
};

net_mem_block_t
net_mem_block_create(
    net_mem_group_t group, uint32_t ep_id, uint32_t capacity, net_mem_alloc_capacity_policy_t policy);

void net_mem_block_free(net_mem_block_t mem_bloc);
void net_mem_block_real_free(net_mem_block_t mem_block);

void net_mem_block_erase(net_mem_block_t block, uint32_t size);
void net_mem_block_append(net_mem_block_t block, void const * data, uint32_t size);

void net_mem_block_link_endpoint(net_mem_block_t block, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
void net_mem_block_link_progress(net_mem_block_t block, net_progress_t progress);
void net_mem_block_unlink(net_mem_block_t block);
                             
#endif
