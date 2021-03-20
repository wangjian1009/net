#include "prometheus_manager_i.h"

prometheus_manager_t prometheus_manager_create(
    mem_allocrator_t alloc, error_monitor_t em)
{
    prometheus_manager_t mgr = mem_alloc(alloc, sizeof(struct prometheus_manager));
    if (mgr == NULL) {
        CPE_ERROR(em, "prometheus: manager: alloc fail!");
        return NULL;
    }

    mgr->m_alloc = alloc;
    mgr->m_em = em;
    
    return mgr;
}

void prometheus_manager_free(prometheus_manager_t mgr) {
    mem_free(mgr->m_alloc, mgr);
}
