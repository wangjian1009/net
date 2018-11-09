#include "net_statistics_reportor_i.h"
#include "net_statistics_reportor_type_i.h"

net_statistics_reportor_t
net_statistics_reportor_create(net_statistics_t statistics, net_statistics_reportor_type_t type) {
    net_statistics_reportor_t reportor = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_reportor));
    if (reportor == NULL) {
        CPE_ERROR(statistics->m_em, "statistics: reportor %s: alloc fail", type->m_name);
        return NULL;
    }

    reportor->m_statistics = statistics;
    reportor->m_type = type;

    if (type->m_init_fun(type->m_ctx, reportor) != 0) {
        mem_free(statistics->m_alloc, reportor);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&statistics->m_reportors, reportor, m_next_for_statistics);
    TAILQ_INSERT_TAIL(&type->m_reportors, reportor, m_next_for_type);
    
    return reportor;
}

void net_statistics_reportor_free(net_statistics_reportor_t reportor) {
    net_statistics_t statistics = reportor->m_statistics;
    net_statistics_reportor_type_t type = reportor->m_type;
        
    type->m_fini_fun(type->m_ctx, reportor);
    
    TAILQ_REMOVE(&statistics->m_reportors, reportor, m_next_for_statistics);
    TAILQ_REMOVE(&type->m_reportors, reportor, m_next_for_type);

    mem_free(statistics->m_alloc, reportor);
}



