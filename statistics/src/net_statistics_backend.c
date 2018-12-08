#include "net_statistics_backend_i.h"

net_statistics_backend_t
net_statistics_backend_create(
    net_statistics_t statistics,
    void * ctx,
    net_statistics_log_event_fun_t log_event,
    net_statistics_log_metric_for_count_fun_t log_metric_for_count,
    net_statistics_log_metric_for_duration_fun_t log_metric_for_duration)
{
    net_statistics_backend_t backend = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_backend));
    if (backend == NULL) {
        CPE_ERROR(statistics->m_em, "statistics: backend: alloc fail");
        return NULL;
    }

    backend->m_statistics = statistics;
    backend->m_ctx = ctx;
    backend->m_log_event = log_event;
    backend->m_log_metric_for_count = log_metric_for_count;
    backend->m_log_metric_for_duration = log_metric_for_duration;

    TAILQ_INSERT_TAIL(&statistics->m_backends, backend, m_next_for_statistics);
    
    return backend;
}

void net_statistics_backend_free(net_statistics_backend_t backend) {
    net_statistics_t statistics = backend->m_statistics;
        
    TAILQ_REMOVE(&statistics->m_backends, backend, m_next_for_statistics);

    mem_free(statistics->m_alloc, backend);
}



