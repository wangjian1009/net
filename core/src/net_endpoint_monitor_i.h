#ifndef NET_ENDPOINT_MONITOR_I_H_INCLEDED
#define NET_ENDPOINT_MONITOR_I_H_INCLEDED
#include "net_endpoint_monitor.h"
#include "net_endpoint_i.h"

NET_BEGIN_DECL

struct net_endpoint_monitor {
    net_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_endpoint_monitor) m_next;
    void * m_ctx;
    void (*m_ctx_free)(void *);
    net_endpoint_monitor_state_changed_fun_t m_on_state_change;
    uint8_t m_is_processing;
    uint8_t m_is_free;
};

void net_endpoint_monitor_real_free(net_endpoint_monitor_t monitor);

NET_END_DECL

#endif
