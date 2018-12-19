#ifndef NET_CONTROL_I_H_INCLEDED
#define NET_CONTROL_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/hash.h"
#include "net_statistics.h"

CPE_BEGIN_DECL

typedef struct net_statistics_transaction_backend * net_statistics_transaction_backend_t;
typedef TAILQ_HEAD(net_statistics_backend_list, net_statistics_backend) net_statistics_backend_list_t;
typedef TAILQ_HEAD(net_statistics_transaction_list, net_statistics_transaction) net_statistics_transaction_list_t;
typedef TAILQ_HEAD(net_statistics_transaction_backend_list, net_statistics_transaction_backend) net_statistics_transaction_backend_list_t;

struct net_statistics {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;

    uint16_t m_transaction_capacity;
    net_statistics_backend_list_t m_backends;
    net_statistics_transaction_list_t m_transactions;

    net_statistics_transaction_list_t m_free_transactions;
    net_statistics_transaction_backend_list_t m_free_transaction_backends;
};

CPE_END_DECL

#endif

