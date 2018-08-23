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

    TAILQ_INSERT_TAIL(&setup->m_conditions, condition, m_next);
    
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

static uint8_t net_debug_condition_cmp_address_r(net_schedule_t schedule, net_address_t address, net_address_t cur_check) {
    if (net_address_cmp_without_port(address, cur_check) == 0) return 1;

    if (net_address_type(cur_check) == net_address_domain) {
        if (schedule->m_dns_local_query) {
            struct net_address_it resolved_it;

            if (net_schedule_dns_local_query(schedule, (const char*)net_address_data(cur_check), &resolved_it, 0) ==0) {
                net_address_t alias_address;

                while((alias_address = net_address_it_next(&resolved_it))) {
                    if (net_debug_condition_cmp_address_r(schedule, address, alias_address)) return 1;
                }
            }
        }
    }
    
    return 0;
}

uint8_t net_debug_condition_check(net_debug_condition_t condition, net_address_t address) {
    switch(condition->m_type) {
    case net_debug_condition_address:
        if (net_address_port(condition->m_address.m_address) != 0
            && net_address_port(condition->m_address.m_address) != net_address_port(address))
        {
            return 0;
        }

        return net_debug_condition_cmp_address_r(condition->m_setup->m_schedule, address, condition->m_address.m_address);
    }
}
