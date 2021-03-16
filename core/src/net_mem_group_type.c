#include "cpe/utils/string_utils.h"
#include "net_mem_group_type_i.h"
#include "net_mem_group_i.h"

net_mem_group_type_t
net_mem_group_type_create(
    net_schedule_t schedule, net_mem_type_t mem_type, 
    uint32_t capacity,
    net_mem_group_type_init_fun_t init_fun,
    net_mem_group_type_fini_fun_t fini_fun,
    net_mem_gruop_type_suggest_size_fun_t suggest_size,
    /*block*/
    net_mem_block_alloc_fun_t block_alloc,
    net_mem_block_free_fun_t block_free)
{
    net_mem_group_type_t type = mem_alloc(schedule->m_alloc, sizeof(struct net_mem_group_type) + capacity);
    if (type == NULL) {
        CPE_ERROR(schedule->m_em, "net: core: mem group type: %s: alloc fail", net_mem_type_str(mem_type));
        return NULL;
    }

    type->m_schedule = schedule;
    type->m_type = mem_type;
    TAILQ_INIT(&type->m_groups);
    type->m_fini_fun = fini_fun;
    type->m_suggest_size = suggest_size;
    type->m_block_alloc = block_alloc;
    type->m_block_free = block_free;

    if (init_fun && init_fun(type) != 0) {
        CPE_ERROR(schedule->m_em, "net: core: mem group type: %s: init fail", net_mem_type_str(mem_type));
        mem_free(schedule->m_alloc, type);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&schedule->m_mem_group_types, type, m_next);
    return type;
}

void net_mem_group_type_free(net_mem_group_type_t type) {
    net_schedule_t schedule = type->m_schedule;

    while(!TAILQ_EMPTY(&type->m_groups)) {
        net_mem_group_free(TAILQ_FIRST(&type->m_groups));
    }

    if (type->m_fini_fun) type->m_fini_fun(type);

    if (schedule->m_dft_mem_group_type == type) {
        schedule->m_dft_mem_group_type = NULL;
    }
    
    TAILQ_REMOVE(&schedule->m_mem_group_types, type, m_next);
    mem_free(schedule->m_alloc, type);
}

void * net_mem_group_type_data(net_mem_group_type_t type) {
    return type + 1;
}
