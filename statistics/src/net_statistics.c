#include "net_statistics_i.h"
#include "net_statistics_backend_i.h"
#include "net_statistics_transaction_i.h"
#include "net_statistics_transaction_backend_i.h"

net_statistics_t net_statistics_create(mem_allocrator_t alloc, error_monitor_t em) {
    net_statistics_t statistics = mem_alloc(alloc, sizeof(struct net_statistics));
    if (statistics == NULL) {
        CPE_ERROR(em, "net: statistics: alloc fail!");
        return NULL;
    }

    statistics->m_alloc = alloc;
    statistics->m_em = em;
    statistics->m_debug = 0;
    statistics->m_transaction_capacity = 0;

    TAILQ_INIT(&statistics->m_backends);
    TAILQ_INIT(&statistics->m_transactions);

    TAILQ_INIT(&statistics->m_free_transactions);
    TAILQ_INIT(&statistics->m_free_transaction_backends);

    return statistics;
}

void net_statistics_free(net_statistics_t statistics) {
    while(!TAILQ_EMPTY(&statistics->m_transactions)) {
        net_statistics_transaction_free(TAILQ_FIRST(&statistics->m_transactions));
    }

    while(!TAILQ_EMPTY(&statistics->m_backends)) {
        net_statistics_backend_free(TAILQ_FIRST(&statistics->m_backends));
    }

    while(!TAILQ_EMPTY(&statistics->m_free_transaction_backends)) {
        net_statistics_transaction_backend_real_free(TAILQ_FIRST(&statistics->m_free_transaction_backends));
    }
    
    while(!TAILQ_EMPTY(&statistics->m_free_transactions)) {
        net_statistics_transaction_real_free(TAILQ_FIRST(&statistics->m_free_transactions));
    }
    
    mem_free(statistics->m_alloc, statistics);
}

uint8_t net_statistics_debug(net_statistics_t statistics) {
    return statistics->m_debug;
}

void net_statistics_set_debug(net_statistics_t statistics, uint8_t debug) {
    statistics->m_debug = debug;
}

void net_statistics_log_metric_for_count(net_statistics_t statistics, const char *name, int quantity) {
    net_statistics_backend_t backend;

    TAILQ_FOREACH(backend, &statistics->m_backends, m_next_for_statistics) {
        backend->m_log_metric_for_count(backend, name, quantity);
    }
}

void net_statistics_log_metric_for_duration(net_statistics_t statistics, const char *name, uint64_t duration_ms) {
    net_statistics_backend_t backend;

    TAILQ_FOREACH(backend, &statistics->m_backends, m_next_for_statistics) {
        backend->m_log_metric_for_duration(backend, name, duration_ms);
    }
}

void net_statistics_log_event(net_statistics_t statistics, const char *name, const char *data) {
    net_statistics_backend_t backend;

    TAILQ_FOREACH(backend, &statistics->m_backends, m_next_for_statistics) {
        backend->m_log_event(backend, name, data);
    }
}

void net_statistics_log_error(net_statistics_t statistics, const char *name, const char *data) {
    net_statistics_backend_t backend;

    TAILQ_FOREACH(backend, &statistics->m_backends, m_next_for_statistics) {
        backend->m_log_error(backend, name, data);
    }
}
