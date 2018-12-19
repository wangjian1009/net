#include "net_statistics_transaction_i.h"
#include "net_statistics_backend_i.h"
#include "net_statistics_transaction_backend_i.h"

net_statistics_transaction_t
net_statistics_transaction_create(net_statistics_t statistics, const char * type, const char * name) {
    net_statistics_transaction_t transaction = TAILQ_FIRST(&statistics->m_free_transactions);

    if (transaction) {
        TAILQ_REMOVE(&statistics->m_free_transactions, transaction, m_next_for_statistics);
    }
    else {
        transaction = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_transaction) + statistics->m_transaction_capacity);
        if (transaction == NULL) {
            CPE_ERROR(statistics->m_em, "statistics: transaction: alloc fail");
            return NULL;
        }
    }
    
    transaction->m_statistics = statistics;
    TAILQ_INIT(&transaction->m_backends);

    uint16_t start_pos = 0;
    net_statistics_backend_t backend;
    TAILQ_FOREACH(backend, &statistics->m_backends, m_next_for_statistics) {
        if (backend->m_transaction_init == NULL) continue;

        net_statistics_transaction_backend_t tb = net_statistics_transaction_backend_create(transaction, backend, start_pos, type, name);
        if (tb == NULL) {
            CPE_ERROR(statistics->m_em, "statistics: transaction: %s init fail", backend->m_name);
            
            while(TAILQ_EMPTY(&transaction->m_backends)) {
                net_statistics_transaction_backend_free(TAILQ_FIRST(&transaction->m_backends));
            }

            TAILQ_INSERT_TAIL(&statistics->m_free_transactions, transaction, m_next_for_statistics);
            return NULL;
        }

        start_pos += backend->m_backend_capacity;
    }
    
    TAILQ_INSERT_TAIL(&statistics->m_transactions, transaction, m_next_for_statistics);
    
    return transaction;
}

void net_statistics_transaction_free(net_statistics_transaction_t transaction) {
    net_statistics_t statistics = transaction->m_statistics;

    while(TAILQ_EMPTY(&transaction->m_backends)) {
        net_statistics_transaction_backend_free(TAILQ_FIRST(&transaction->m_backends));
    }
    
    TAILQ_REMOVE(&statistics->m_transactions, transaction, m_next_for_statistics);

    TAILQ_INSERT_TAIL(&statistics->m_free_transactions, transaction, m_next_for_statistics);
}

void net_statistics_transaction_real_free(net_statistics_transaction_t transaction) {
    net_statistics_t statistics = transaction->m_statistics;

    TAILQ_REMOVE(&statistics->m_free_transactions, transaction, m_next_for_statistics);
    
    mem_free(statistics->m_alloc, transaction);
}

void net_statistics_transaction_set_state(net_statistics_transaction_t transaction, const char * state) {
    net_statistics_transaction_backend_t tb;
    TAILQ_FOREACH(tb, &transaction->m_backends, m_next_for_transaction) { 
        if (tb->m_backend->m_transaction_set_state) {
            tb->m_backend->m_transaction_set_state(
                tb->m_backend, tb->m_transaction,
                ((uint8_t *)(transaction + 1)) + tb->m_start_pos,
                state);
        }
    }
}
