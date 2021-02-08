#ifndef NET_MEM_GROUP_I_H_INCLEDED
#define NET_MEM_GROUP_I_H_INCLEDED
#include "net_mem_group.h"
#include "net_mem_group_type_i.h"

struct net_mem_group {
    net_mem_group_type_t m_type;
    TAILQ_ENTRY(net_mem_group) m_next;
    net_mem_block_list_t m_blocks;
};

void net_mem_group_real_free(net_mem_group_t mem_group);

#endif
