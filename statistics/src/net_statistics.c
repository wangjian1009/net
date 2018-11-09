#include "net_statistics_i.h"
#include "net_statistics_reportor_i.h"
#include "net_statistics_reportor_type_i.h"

net_statistics_t net_statistics_create(mem_allocrator_t alloc, error_monitor_t em) {
    net_statistics_t statistics = mem_alloc(alloc, sizeof(struct net_statistics));
    if (statistics == NULL) {
        CPE_ERROR(em, "net: statistics: alloc fail!");
        return NULL;
    }

    statistics->m_alloc = alloc;
    statistics->m_em = em;
    statistics->m_debug = 0;

    TAILQ_INIT(&statistics->m_reportor_types);
    TAILQ_INIT(&statistics->m_reportors);
    
    return statistics;
}

void net_statistics_free(net_statistics_t statistics) {
    while(!TAILQ_EMPTY(&statistics->m_reportors)) {
        net_statistics_reportor_free(TAILQ_FIRST(&statistics->m_reportors));
    }

    while(!TAILQ_EMPTY(&statistics->m_reportor_types)) {
        net_statistics_reportor_type_free(TAILQ_FIRST(&statistics->m_reportor_types));
    }
    
    mem_free(statistics->m_alloc, statistics);
}

uint8_t net_statistics_debug(net_statistics_t statistics) {
    return statistics->m_debug;
}

void net_statistics_set_debug(net_statistics_t statistics, uint8_t debug) {
    statistics->m_debug = debug;
}
