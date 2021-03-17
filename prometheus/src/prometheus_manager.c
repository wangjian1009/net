#include "net_schedule.h"
#include "prometheus_manager_i.h"

prometheus_manager_t prometheus_manager_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    prometheus_manager_t mgr = mem_alloc(alloc, sizeof(struct prometheus_manager));
    if (mgr == NULL) {
        CPE_ERROR(em, "prometheus: manager: alloc fail!");
        return NULL;
    }

    mgr->m_alloc = alloc;
    mgr->m_em = em;
    mgr->m_schedule = schedule;
    mgr->m_driver = driver;
    
    return mgr;
}

void prometheus_manager_free(prometheus_manager_t mgr) {
    mem_free(mgr->m_alloc, mgr);
}
