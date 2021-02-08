#include "net_mem_group_i.h"
#include "net_mem_group_type_i.h"
#include "net_mem_block_i.h"

net_mem_group_t
net_mem_group_create(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;
    
    net_mem_group_t mem_group = TAILQ_FIRST(&schedule->m_free_mem_groups);
    if (mem_group) {
        TAILQ_REMOVE(&schedule->m_free_mem_groups, mem_group, m_next);
    }
    else {
        mem_group = mem_alloc(schedule->m_alloc, sizeof(struct net_mem_group));
        if (mem_group == NULL) {
            CPE_ERROR(schedule->m_em, "core: mem group: alloc fail!");
            return NULL;
        }
    }

    mem_group->m_type = type;
    TAILQ_INIT(&mem_group->m_blocks);

    TAILQ_INSERT_TAIL(&type->m_groups, mem_group, m_next);
    return mem_group;
}

void net_mem_group_free(net_mem_group_t mem_group) {
    net_mem_group_type_t type = mem_group->m_type;
    net_schedule_t schedule = type->m_schedule;

    while(!TAILQ_EMPTY(&mem_group->m_blocks)) {
        net_mem_block_free(TAILQ_FIRST(&mem_group->m_blocks));
    }

    if (schedule->m_dft_mem_group == mem_group) {
        schedule->m_dft_mem_group = NULL;
    }

    TAILQ_REMOVE(&type->m_groups, mem_group, m_next);
    
    mem_group->m_type = (void*)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_mem_groups, mem_group, m_next);
}

void net_mem_group_real_free(net_mem_group_t mem_group) {
    net_schedule_t schedule = (net_schedule_t)mem_group->m_type;

    TAILQ_REMOVE(&schedule->m_free_mem_groups, mem_group, m_next);

    mem_free(schedule->m_alloc, mem_group);
}
