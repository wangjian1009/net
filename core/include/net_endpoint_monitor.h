#ifndef NET_ENDPOINT_MONITOR_H_INCLEDED
#define NET_ENDPOINT_MONITOR_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef void (*net_endpoint_monitor_state_changed_fun_t)(void * ctx, net_endpoint_t endpoint, net_endpoint_state_t from_state);

net_endpoint_monitor_t
net_endpoint_monitor_create(
    net_endpoint_t endpoint,
    void * ctx,
    void (*ctx_free)(void * ctx),
    net_endpoint_monitor_state_changed_fun_t on_state_change);

void net_endpoint_monitor_free(net_endpoint_monitor_t endpoint);

NET_END_DECL

#endif
