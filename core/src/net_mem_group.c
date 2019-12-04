#include "net_mem_group_i.h"
#include "net_mem_block_i.h"

net_mem_group_t
net_mem_group_create(net_schedule_t schedule, uint32_t capacity) {
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

    mem_group->m_schedule = schedule;
    TAILQ_INIT(&mem_group->m_blocks);

    return mem_group;
}

void net_mem_group_free(net_mem_group_t mem_group) {
    net_schedule_t schedule = mem_group->m_schedule;

    while(!TAILQ_EMPTY(&mem_group->m_blocks)) {
        net_mem_block_free(TAILQ_FIRST(&mem_group->m_blocks));
    }

    TAILQ_INSERT_TAIL(&schedule->m_free_mem_groups, mem_group, m_next);
}

void net_mem_group_real_free(net_mem_group_t mem_group) {
    net_schedule_t schedule = mem_group->m_schedule;

    TAILQ_REMOVE(&schedule->m_free_mem_groups, mem_group, m_next);

    mem_free(schedule->m_alloc, mem_group);
}
