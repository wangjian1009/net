#include "net_statistics_transaction_i.h"

net_statistics_transaction_t
net_statistics_transaction_create(net_statistics_t statistics) {
    net_statistics_transaction_t transaction = mem_alloc(statistics->m_alloc, sizeof(struct net_statistics_transaction));
    if (transaction == NULL) {
        CPE_ERROR(statistics->m_em, "statistics: transaction: alloc fail");
        return NULL;
    }

    transaction->m_statistics = statistics;

    TAILQ_INSERT_TAIL(&statistics->m_transactions, transaction, m_next_for_statistics);
    
    return transaction;
}

void net_statistics_transaction_free(net_statistics_transaction_t transaction) {
    net_statistics_t statistics = transaction->m_statistics;
        
    TAILQ_REMOVE(&statistics->m_transactions, transaction, m_next_for_statistics);

    mem_free(statistics->m_alloc, transaction);
}



