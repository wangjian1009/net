#include "net_mem_group_i.h"

net_mem_group_t net_mem_group_create(net_schedule_t schedule, uint32_t capacity) {
    net_mem_group_t mem_group = mem_alloc(schedule->m_alloc, sizeof(struct net_mem_group));

    return mem_group;
}

void net_mem_group_free(net_mem_group_t mem_group) {
}

