#ifndef NET_STATISTICS_TRANSACTION_BACKEND_I_H_INCLEDED
#define NET_STATISTICS_TRANSACTION_BACKEND_I_H_INCLEDED
#include "net_statistics_backend_i.h"
#include "net_statistics_transaction_i.h"

CPE_BEGIN_DECL

struct net_statistics_transaction_backend {
    net_statistics_transaction_t m_transaction;
    TAILQ_ENTRY(net_statistics_transaction_backend) m_next_for_transaction;
    net_statistics_backend_t m_backend;
    TAILQ_ENTRY(net_statistics_transaction_backend) m_next_for_backend;
    uint16_t m_start_pos;
};

net_statistics_transaction_backend_t
net_statistics_transaction_backend_create(
    net_statistics_transaction_t transaction, net_statistics_backend_t backend, uint16_t start_pos, const char * type);
void net_statistics_transaction_backend_free(net_statistics_transaction_backend_t transaction_backend);

void net_statistics_transaction_backend_real_free(net_statistics_transaction_backend_t transaction_backend);

CPE_END_DECL

#endif

