#ifndef NET_ENDPOINT_MONITOR_H_INCLEDED
#define NET_ENDPOINT_MONITOR_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

enum net_endpoint_monitor_evt_type {
    net_endpoint_monitor_evt_state_changed,
    net_endpoint_monitor_evt_write_blocked,
    net_endpoint_monitor_evt_write_unblocked,
};

struct net_endpoint_monitor_evt {
    net_endpoint_monitor_evt_type_t m_type;
    union {
        net_endpoint_state_t m_from_state;
    };
};

typedef void (*net_endpoint_monitor_cb_fun_t)(void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

net_endpoint_monitor_t
net_endpoint_monitor_create(
    net_endpoint_t endpoint,
    void * ctx,
    void (*ctx_free)(void * ctx),
    net_endpoint_monitor_cb_fun_t on_state_change);

void net_endpoint_monitor_free(net_endpoint_monitor_t endpoint);

void net_endpoint_monitor_free_by_ctx(net_endpoint_t endpoint, void * ctx);

NET_END_DECL

#endif
