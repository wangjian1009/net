#include "net_statistics_i.h"

net_statistics_t net_statistics_create(mem_allocrator_t alloc, error_monitor_t em) {
    net_statistics_t statistics = mem_alloc(alloc, sizeof(struct net_statistics));
    if (statistics == NULL) {
        CPE_ERROR(em, "net: statistics: alloc fail!");
        return NULL;
    }

    statistics->m_alloc = alloc;
    statistics->m_em = em;

    return statistics;
}

void net_statistics_free(net_statistics_t statistics) {
    mem_free(statistics->m_alloc, statistics);
}

uint8_t net_statistics_debug(net_statistics_t statistics) {
    return statistics->m_debug;
}

void net_statistics_set_debug(net_statistics_t statistics, uint8_t debug) {
    statistics->m_debug = debug;
}
