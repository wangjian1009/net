#ifndef NET_DEBUG_CONDITION_H_INCLEDED
#define NET_DEBUG_CONDITION_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_debug_condition_type {
    net_debug_condition_address
} net_debug_condition_type_t;

typedef enum net_debug_condition_scope {
    net_debug_condition_scope_local,
    net_debug_condition_scope_remote,
} net_debug_condition_scope_t;

net_debug_condition_t
net_debug_condition_address_create(net_debug_setup_t debug_setup, net_debug_condition_scope_t scope, net_address_t address);

void net_debug_condition_free(net_debug_condition_t debug_condition);

NET_END_DECL

#endif
