#ifndef NET_PING_TYPES_H_INCLEDED
#define NET_PING_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_ping_mgr * net_ping_mgr_t;
typedef struct net_ping_task * net_ping_task_t;
typedef struct net_ping_record * net_ping_record_t;
typedef struct net_ping_record_it * net_ping_record_it_t;

typedef enum net_ping_error {
    net_ping_error_none,
    net_ping_error_no_network,
    net_ping_error_no_right,
    net_ping_error_internal,
} net_ping_error_t;

typedef enum net_ping_task_state {
    net_ping_task_state_init,
    net_ping_task_state_processing,
    net_ping_task_state_done,
    net_ping_task_state_error,
} net_ping_task_state_t;

typedef enum net_ping_type net_ping_type_t;

NET_END_DECL

#endif
