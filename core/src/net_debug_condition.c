#include "net_address.h"
#include "net_debug_condition_i.h"

net_debug_condition_t
net_debug_condition_address_create(net_debug_setup_t setup, net_debug_condition_scope_t scope, net_address_t address) {
    net_schedule_t schedule = setup->m_schedule;
    
    net_debug_condition_t condition = mem_alloc(schedule->m_alloc, sizeof(struct net_debug_condition));
    if (condition == NULL) {
        CPE_ERROR(schedule->m_em, "core: net_debug_condition: alloc fail!");
        return NULL;
    }

    condition->m_setup = setup;
    condition->m_scope = scope;
    condition->m_type = net_debug_condition_address;
    condition->m_address.m_address = net_address_copy(schedule, address);
    if (condition->m_address.m_address == NULL) {
        CPE_ERROR(schedule->m_em, "core: net_debug_condition: dup address fail!");
        mem_free(schedule->m_alloc, condition);
        return NULL;
    }
    
    return condition;
}

void net_debug_condition_free(net_debug_condition_t condition) {
    switch(condition->m_type) {
    case net_debug_condition_address:
        net_address_free(condition->m_address.m_address);
        break;
    }

    TAILQ_REMOVE(&condition->m_setup->m_conditions, condition, m_next);

    mem_free(condition->m_setup->m_schedule->m_alloc, condition);
}

uint8_t net_debug_condition_check(net_debug_condition_t condition, net_address_t address) {
    switch(condition->m_type) {
    case net_debug_condition_address:
        if (net_address_cmp_without_port(condition->m_address.m_address, address) != 0) return 0;
        
        if (net_address_port(condition->m_address.m_address) != 0
            && net_address_port(condition->m_address.m_address) != net_address_port(address))
        {
            return 0;
        }
        return 1;
    }
}
