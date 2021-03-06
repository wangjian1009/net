#ifndef NET_MEM_GROUP_TYPE_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_I_H_INCLEDED
#include "net_mem_group_type.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_mem_group_type {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_mem_group_type) m_next;
    net_mem_type_t m_type;
    net_mem_group_type_fini_fun_t m_fini_fun;
    net_mem_gruop_type_suggest_size_fun_t m_suggest_size;
    net_mem_block_alloc_fun_t m_block_alloc;
    net_mem_block_free_fun_t m_block_free;
    net_mem_block_update_ep_fun_t m_block_update_ep;
    net_mem_group_list_t m_groups;
};

void * net_mem_group_type_data(net_mem_group_type_t type);

NET_END_DECL

#endif
