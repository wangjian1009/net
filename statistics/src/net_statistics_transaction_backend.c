#include "net_statistics_transaction_backend_i.h"

net_statistics_transaction_backend_t
net_statistics_transaction_backend_create(
    net_statistics_transaction_t transaction, net_statistics_backend_t backend, uint16_t start_pos, const char * type, const char * name)
{
    net_statistics_t statistics = backend->m_statistics;
    
    net_statistics_transaction_backend_t tb = TAILQ_FIRST(&statistics->m_free_transaction_backends);
    if (tb) {
        TAILQ_REMOVE(&statistics->m_free_transaction_backends, tb, m_next_for_transaction);
    }
    else {
        tb = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_transaction_backend));
        if (tb == NULL) {
            CPE_ERROR(statistics->m_em, "statistics: transaction_backend: alloc fail");
            return NULL;
        }
    }

    tb->m_transaction = transaction;
    tb->m_backend = backend;
    tb->m_start_pos = start_pos; 
    TAILQ_INSERT_TAIL(&tb->m_transaction->m_backends, tb, m_next_for_transaction);
    TAILQ_INSERT_TAIL(&tb->m_backend->m_transactions, tb, m_next_for_backend);

    if (backend->m_transaction_init(backend, transaction, ((uint8_t*)(transaction + 1)) + start_pos, type, name) != 0) {
        tb->m_transaction = (net_statistics_transaction_t)statistics;
        TAILQ_INSERT_TAIL(&statistics->m_free_transaction_backends, tb, m_next_for_transaction);
        return NULL;
    }
    
    return tb;
}

void net_statistics_transaction_backend_free(net_statistics_transaction_backend_t tb) {
    net_statistics_transaction_t transaction = tb->m_transaction;
    net_statistics_backend_t backend = tb->m_backend;
    net_statistics_t statistics = backend->m_statistics;

    if (backend->m_transaction_fini) {
        backend->m_transaction_fini(backend, transaction, ((uint8_t *)(transaction + 1)) + tb->m_start_pos);
    }

    TAILQ_REMOVE(&tb->m_transaction->m_backends, tb, m_next_for_transaction);
    TAILQ_REMOVE(&tb->m_backend->m_transactions, tb, m_next_for_backend);
    
    tb->m_transaction = (net_statistics_transaction_t)statistics;
    TAILQ_INSERT_TAIL(&statistics->m_free_transaction_backends, tb, m_next_for_transaction);
}

void net_statistics_transaction_backend_real_free(net_statistics_transaction_backend_t transaction_backend) {
    net_statistics_t statistics = (net_statistics_t)transaction_backend->m_transaction;
    TAILQ_REMOVE(&statistics->m_free_transaction_backends, transaction_backend, m_next_for_transaction);
    mem_free(statistics->m_alloc, transaction_backend);
}

