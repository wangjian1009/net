#ifndef NET_DNS_SYSTEM_H_INCLEDED
#define NET_DNS_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_dns_mode {
    net_dns_ipv4_first,
    net_dns_ipv6_first,
} net_dns_mode_t;

typedef struct net_dns_manage * net_dns_manage_t;
typedef struct net_dns_dgram_receiver * net_dns_dgram_receiver_t;
typedef struct net_dns_entry * net_dns_entry_t;
typedef struct net_dns_entry_item * net_dns_entry_item_t;
typedef struct net_dns_entry_item_it * net_dns_entry_item_it_t;
typedef struct net_dns_task * net_dns_task_t;
typedef struct net_dns_task_step * net_dns_task_step_t;
typedef struct net_dns_task_ctx * net_dns_task_ctx_t;
typedef struct net_dns_task_builder * net_dns_task_builder_t;
typedef struct net_dns_source * net_dns_source_t;
typedef struct net_dns_source_it * net_dns_source_it_t;

typedef struct net_dns_source_nameserver * net_dns_source_nameserver_t;

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
    
NET_END_DECL

#endif
