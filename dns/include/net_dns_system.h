#ifndef NET_DNS_SYSTEM_H_INCLEDED
#define NET_DNS_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_dns_mode {
    net_dns_ipv4_first,
    net_dns_ipv6_first,
} net_dns_mode_t;

typedef struct net_dns_manage * net_dns_manage_t;
typedef struct net_dns_scope * net_dns_scope_t;
typedef struct net_dns_scope_it * net_dns_scope_it_t;
typedef struct net_dns_entry * net_dns_entry_t;
typedef struct net_dns_entry_it * net_dns_entry_it_t;
typedef struct net_dns_entry_item * net_dns_entry_item_t;
typedef struct net_dns_entry_item_it * net_dns_entry_item_it_t;
typedef struct net_dns_task * net_dns_task_t;
typedef struct net_dns_task_step * net_dns_task_step_t;
typedef struct net_dns_task_ctx * net_dns_task_ctx_t;
typedef struct net_dns_task_ctx_it * net_dns_task_ctx_it_t;
typedef struct net_dns_task_builder * net_dns_task_builder_t;
typedef struct net_dns_source * net_dns_source_t;
typedef struct net_dns_source_it * net_dns_source_it_t;

typedef struct net_dns_source_ns * net_dns_source_ns_t;

typedef enum net_dns_entry_state {
    net_dns_entry_runing,
    net_dns_entry_done,
} net_dns_entry_state_t;

typedef enum net_dns_task_state {
    net_dns_task_state_init,
    net_dns_task_state_runing,
    net_dns_task_state_success,
    net_dns_task_state_error,
} net_dns_task_state_t;

typedef enum net_dns_item_select_policy {
    net_dns_item_select_policy_first,
    net_dns_item_select_policy_max_ttl,
} net_dns_item_select_policy_t;

typedef enum net_dns_ns_trans_type {
    net_dns_trans_udp,
    net_dns_trans_tcp,
} net_dns_ns_trans_type_t;

NET_END_DECL

#endif
