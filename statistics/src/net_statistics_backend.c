#include "cpe/utils/string_utils.h"
#include "net_statistics_backend_i.h"
#include "net_statistics_transaction_i.h"
#include "net_statistics_transaction_backend_i.h"

net_statistics_backend_t
net_statistics_backend_create(
    net_statistics_t statistics,
    const char * name,
    uint16_t backend_capacity,
    net_statistics_backend_init_fun_t backend_init,
    net_statistics_backend_fini_fun_t backend_fini,
    net_statistics_log_event_fun_t log_event,
    net_statistics_log_error_fun_t log_error,
    net_statistics_log_metric_for_count_fun_t log_metric_for_count,
    net_statistics_log_metric_for_duration_fun_t log_metric_for_duration,
    uint16_t transaction_capacity,
    net_statistics_transaction_init_fun_t transaction_init,
    net_statistics_transaction_fini_fun_t transaction_fini,
    net_statistics_transaction_set_state_fun_t transaction_set_state)
{
    if (transaction_capacity > 0 && !TAILQ_EMPTY(&statistics->m_transactions)) {
        CPE_ERROR(statistics->m_em, "statistics: backend: already have transactions");
        return NULL;
    }
    
    net_statistics_backend_t backend = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_backend) + backend_capacity);
    if (backend == NULL) {
        CPE_ERROR(statistics->m_em, "statistics: backend: alloc fail");
        return NULL;
    }

    backend->m_statistics = statistics;
    cpe_str_dup(backend->m_name, sizeof(backend->m_name), name);
    backend->m_backend_capacity = backend_capacity;
    backend->m_backend_init = backend_init;
    backend->m_backend_fini = backend_fini;
    backend->m_log_event = log_event;
    backend->m_log_error = log_error;
    backend->m_log_metric_for_count = log_metric_for_count;
    backend->m_log_metric_for_duration = log_metric_for_duration;
    backend->m_transaction_capacity = transaction_capacity;
    backend->m_transaction_init = transaction_init;
    backend->m_transaction_fini = transaction_fini;
    backend->m_transaction_set_state = transaction_set_state;
    TAILQ_INIT(&backend->m_transactions);

    statistics->m_transaction_capacity += transaction_capacity;
    TAILQ_INSERT_TAIL(&statistics->m_backends, backend, m_next_for_statistics);
    
    return backend;
}

void net_statistics_backend_free(net_statistics_backend_t backend) {
    net_statistics_t statistics = backend->m_statistics;

    while(!TAILQ_EMPTY(&backend->m_transactions)) {
        net_statistics_transaction_backend_free(TAILQ_FIRST(&backend->m_transactions));
    }
    
    TAILQ_REMOVE(&statistics->m_backends, backend, m_next_for_statistics);

    mem_free(statistics->m_alloc, backend);
}

const char * net_statistics_backend_name(net_statistics_backend_t backend) {
    return backend->m_name;
}

void * net_statistics_backend_data(net_statistics_backend_t backend) {
    return backend + 1;
}

net_statistics_backend_t net_statistics_backend_from_data(void * data) {
    return ((net_statistics_backend_t)data) - 1;
}
