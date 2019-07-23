#include "net_schedule.h"
#include "net_ping_mgr_i.h"
#include "net_ping_task_i.h"
#include "net_ping_record_i.h"
#include "net_ping_processor_i.h"

net_ping_mgr_t net_ping_mgr_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    net_ping_mgr_t mgr = mem_alloc(alloc, sizeof(struct net_ping_mgr));
    if (mgr == NULL) {
        CPE_ERROR(em, "ping: mgr: alloc fail!");
        return NULL;
    }

    mgr->m_alloc = alloc;
    mgr->m_em = em;
    mgr->m_schedule = schedule;
    mgr->m_driver = driver;

    mgr->m_ping_id_max = 0;
    TAILQ_INIT(&mgr->m_ping_tasks);

    TAILQ_INIT(&mgr->m_free_ping_tasks);
    TAILQ_INIT(&mgr->m_free_ping_records);
    TAILQ_INIT(&mgr->m_free_ping_processors);

    return mgr;
}

void net_ping_mgr_free(net_ping_mgr_t mgr) {
    
    while(!TAILQ_EMPTY(&mgr->m_ping_tasks)) {
        net_ping_task_free(TAILQ_FIRST(&mgr->m_ping_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_tasks)) {
        net_ping_task_real_free(TAILQ_FIRST(&mgr->m_free_ping_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_records)) {
        net_ping_record_real_free(TAILQ_FIRST(&mgr->m_free_ping_records));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_ping_processors)) {
        net_ping_processor_real_free(TAILQ_FIRST(&mgr->m_free_ping_processors));
    }
    
    mem_free(mgr->m_alloc, mgr);
}

mem_buffer_t net_ping_mgr_tmp_buffer(net_ping_mgr_t mgr) {
    return net_schedule_tmp_buffer(mgr->m_schedule);
}
