#include "cpe/pal/pal_stdio.h"
#include "net_smux_manager_i.h"

net_smux_manager_t
net_smux_manager_create(net_schedule_t schedule, mem_allocrator_t alloc, error_monitor_t em) {
    net_smux_manager_t manager = mem_alloc(alloc, sizeof(struct net_smux_manager));

    manager->m_alloc = alloc;
    manager->m_em = em;
    manager->m_schedule = schedule;

    return manager;
}

void net_smux_manager_free(net_smux_manager_t smux_manager) {
    
}
