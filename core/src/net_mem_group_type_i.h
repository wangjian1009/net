#ifndef NET_MEM_GROUP_TYPE_I_H_INCLEDED
#define NET_MEM_GROUP_TYPE_I_H_INCLEDED
#include "net_mem_group_type.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_mem_group_type {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_mem_group_type) m_next;
    char m_name[32];
    net_mem_group_list_t m_groups;
};

NET_END_DECL

#endif
