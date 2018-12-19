#ifndef NET_STATISTICS_TRANSACTION_I_H_INCLEDED
#define NET_STATISTICS_TRANSACTION_I_H_INCLEDED
#include "net_statistics_transaction.h"
#include "net_statistics_i.h"

CPE_BEGIN_DECL

struct net_statistics_transaction {
    net_statistics_t m_statistics;
    TAILQ_ENTRY(net_statistics_transaction) m_next_for_statistics;
    net_statistics_transaction_backend_list_t m_backends;
};

void net_statistics_transaction_real_free(net_statistics_transaction_t transaction);

CPE_END_DECL

#endif

