#include "cpe/utils/string_utils.h"
#include "net_mem_group_type_i.h"
#include "net_mem_group_i.h"

net_mem_group_type_t
net_mem_group_type_create(net_schedule_t schedule, const char * name, uint32_t capacity) {
    net_mem_group_type_t type = mem_alloc(schedule->m_alloc, sizeof(struct net_mem_group_type) + capacity);
    if (type == NULL) {
        CPE_ERROR(schedule->m_em, "net: core: mem group type: %s: alloc fail", name);
        return NULL;
    }

    type->m_schedule = schedule;
    cpe_str_dup(type->m_name, sizeof(type->m_name), name);
    TAILQ_INIT(&type->m_groups);

    TAILQ_INSERT_TAIL(&schedule->m_mem_group_types, type, m_next);
    return type;
}

void net_mem_group_type_free(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;

    while(!TAILQ_EMPTY(&type->m_groups)) {
        net_mem_group_free(TAILQ_FIRST(&type->m_groups));
    }

    if (schedule->m_dft_mem_group_type == type) {
        schedule->m_dft_mem_group_type = NULL;
    }
    
    TAILQ_REMOVE(&schedule->m_mem_group_types, type, m_next);
    mem_free(schedule->m_alloc, type);
}

