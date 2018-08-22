#include "net_address.h"
#include "net_debug_setup_i.h"
#include "net_debug_condition_i.h"
#include "net_endpoint_i.h"
#include "net_dgram_i.h"

net_debug_setup_t
net_debug_setup_create(net_schedule_t schedule, uint8_t debug_protocol, uint8_t debug_driver) {
    net_debug_setup_t setup = mem_alloc(schedule->m_alloc, sizeof(struct net_debug_setup));
    if (setup == NULL) {
        CPE_ERROR(schedule->m_em, "core: net_debug_setup: alloc fail!");
        return NULL;
    }

    setup->m_schedule = schedule;
    setup->m_protocol_debug = debug_protocol;
    setup->m_driver_debug = debug_driver;
    TAILQ_INIT(&setup->m_conditions);

    TAILQ_INSERT_TAIL(&setup->m_schedule->m_debug_setups, setup, m_next_for_schedule);
    
    return setup;
}

void net_debug_setup_free(net_debug_setup_t setup) {
    while(!TAILQ_EMPTY(&setup->m_conditions)) {
        net_debug_condition_free(TAILQ_FIRST(&setup->m_conditions));
    }

    TAILQ_REMOVE(&setup->m_schedule->m_debug_setups, setup, m_next_for_schedule);
    
    mem_free(setup->m_schedule->m_alloc, setup);
}

uint8_t net_debug_setup_check_endpoint(net_debug_setup_t setup, net_endpoint_t endpoint) {
    net_debug_condition_t condition;

    TAILQ_FOREACH(condition, &setup->m_conditions, m_next) {
        switch(condition->m_scope) {
        case net_debug_condition_scope_local:
            if (endpoint->m_address == NULL) return 0;
            if (!net_debug_condition_check(condition, endpoint->m_address)) return 0;
            break;
        case net_debug_condition_scope_remote:
            if (endpoint->m_remote_address == NULL) return 0;
            if (!net_debug_condition_check(condition, endpoint->m_remote_address)) return 0;
            break;
        }
    }

    return 1;
}

uint8_t net_debug_setup_check_dgram(net_debug_setup_t setup, net_dgram_t dgram, net_address_t remote) {
    net_debug_condition_t condition;

    TAILQ_FOREACH(condition, &setup->m_conditions, m_next) {
        switch(condition->m_scope) {
        case net_debug_condition_scope_local:
            if (dgram->m_address == NULL) return 0;
            if (!net_debug_condition_check(condition, dgram->m_address)) return 0;
            break;
        case net_debug_condition_scope_remote:
            if (remote == NULL) return 0;
            if (!net_debug_condition_check(condition, remote)) return 0;
            break;
        }
    }

    return 1;
}
