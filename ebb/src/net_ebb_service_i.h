#ifndef NET_EBB_SERVICE_H_I_INCLEDED
#define NET_EBB_SERVICE_H_I_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_ebb_service.h"

typedef TAILQ_HEAD(net_ebb_connection_list, net_ebb_connection) net_ebb_connection_list_t;

struct net_ebb_service {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    uint32_t m_cfg_connection_timeout_ms;
    net_ebb_connection_list_t m_connections;
};

mem_buffer_t net_ebb_service_tmp_buffer(net_ebb_service_t service);

#endif
