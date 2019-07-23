#include "net_icmp_mgr_i.h"
#include "net_icmp_ping_task_i.h"
#include "net_icmp_ping_record_i.h"
#include "net_icmp_ping_processor_i.h"

net_icmp_mgr_t net_icmp_mgr_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule) {
    net_icmp_mgr_t mgr = mem_alloc(alloc, sizeof(struct net_icmp_mgr));
    if (mgr == NULL) {
        CPE_ERROR(em, "icmp: mgr: alloc fail!");
        return NULL;
    }

    mgr->m_alloc = alloc;
    mgr->m_em = em;
    mgr->m_schedule = schedule;
    
    TAILQ_INIT(&mgr->m_ping_tasks);

    TAILQ_INIT(&mgr->m_free_ping_tasks);
    TAILQ_INIT(&mgr->m_free_ping_records);
    TAILQ_INIT(&mgr->m_free_ping_processors);

    return mgr;
}

void net_icmp_mgr_free(net_icmp_mgr_t mgr) {
    
    while(!TAILQ_EMPTY(&mgr->m_ping_tasks)) {
        net_icmp_ping_task_free(TAILQ_FIRST(&mgr->m_ping_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_tasks)) {
        net_icmp_ping_task_real_free(TAILQ_FIRST(&mgr->m_free_ping_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_records)) {
        net_icmp_ping_record_real_free(TAILQ_FIRST(&mgr->m_free_ping_records));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_processors)) {
        net_icmp_ping_processor_real_free(TAILQ_FIRST(&mgr->m_free_ping_processors));
    }
    
    mem_free(mgr->m_alloc, mgr);
}
