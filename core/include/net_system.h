#ifndef NET_SYSTEM_H_INCLEDED
#define NET_SYSTEM_H_INCLEDED
#include "net_common.h"

NET_BEGIN_DECL

#define NET_ENDPOINT_NO_LIMIT ((uint32_t)-1)

typedef struct net_schedule * net_schedule_t;
typedef struct net_mem_block * net_mem_block_t;
typedef struct net_mem_group * net_mem_group_t;
typedef struct net_local_ip_stack_monitor * net_local_ip_stack_monitor_t;
typedef struct net_driver * net_driver_t;
typedef struct net_protocol * net_protocol_t;
typedef struct net_link * net_link_t;
typedef struct net_timer * net_timer_t;
typedef struct net_watcher * net_watcher_t;
typedef struct net_watcher_it * net_watcher_it_t;
typedef struct net_endpoint * net_endpoint_t;
typedef struct net_endpoint_writer * net_endpoint_writer_t;
typedef struct net_endpoint_it * net_endpoint_it_t;
typedef struct net_endpoint_monitor * net_endpoint_monitor_t;
typedef struct net_acceptor * net_acceptor_t;
typedef struct net_dgram * net_dgram_t;
typedef struct net_address * net_address_t;
typedef struct net_address_it * net_address_it_t;
typedef struct net_ipset * net_ipset_t;
typedef struct net_address_matcher * net_address_matcher_t;
typedef struct net_address_rule * net_address_rule_t;
typedef struct net_dns_query * net_dns_query_t;

typedef struct net_address_data_ipv4 * net_address_data_ipv4_t;
typedef struct net_address_data_ipv6 * net_address_data_ipv6_t;

typedef struct net_debug_setup * net_debug_setup_t;
typedef struct net_debug_condition * net_debug_condition_t;

typedef enum net_address_type {
    net_address_ipv4,
    net_address_ipv6,
    net_address_domain,
    net_address_local,
} net_address_type_t;

typedef enum net_endpoint_buf_type {
    net_ep_buf_read,
    net_ep_buf_forward,
    net_ep_buf_write,
    net_ep_buf_user1,
    net_ep_buf_user2,
    net_ep_buf_user3,
    net_ep_buf_count,
} net_endpoint_buf_type_t;

typedef enum net_endpoint_state {
    net_endpoint_state_disable,
    net_endpoint_state_resolving,
    net_endpoint_state_connecting,
    net_endpoint_state_established,
    net_endpoint_state_logic_error,
    net_endpoint_state_network_error,
    net_endpoint_state_deleting,
} net_endpoint_state_t;

typedef enum net_endpoint_option {
    net_endpoint_option_fastopen,
    net_endpoint_option_no_delay,
} net_endpoint_option_t;

typedef enum net_data_direct {
    net_data_in,
    net_data_out,
} net_data_direct_t;

typedef enum net_endpoint_data_event {
    net_endpoint_data_supply,
    net_endpoint_data_consume,
} net_endpoint_data_event_t;

typedef int (*net_endpoint_prepare_connect_fun_t)(void * ctx, net_endpoint_t endpoint, uint8_t * do_connect);

typedef void (*net_endpoint_data_watch_fun_t)(
    void * ctx, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type,
    net_endpoint_data_event_t evt, uint32_t size);

typedef void (*net_endpoint_data_watch_ctx_fini_fun_t)(void*, net_endpoint_t endpoint);

/*acceptor*/
typedef int (*net_acceptor_on_new_endpoint_fun_t)(void * ctx, net_endpoint_t endpoint);

/*monitor*/
typedef void (*net_data_monitor_fun_t) (
    void * ctx, net_endpoint_t endpoint, net_data_direct_t direct, uint32_t sz);

/*dns*/
typedef enum net_dns_query_type {
    net_dns_query_ipv4,
    net_dns_query_ipv6,
    net_dns_query_ipv4v6,
    net_dns_query_domain,
} net_dns_query_type_t;
typedef void (*net_dns_query_callback_fun_t)(void * ctx, net_address_t main_address, net_address_it_t all_address);
typedef void (*net_dns_query_ctx_free_fun_t)(void * ctx);

/*ipstack*/
typedef enum net_local_ip_stack {
	net_local_ip_stack_none = 0,
    net_local_ip_stack_ipv4 = 1,
    net_local_ip_stack_ipv6 = 2,
	net_local_ip_stack_dual = 3,
} net_local_ip_stack_t;

typedef enum net_endpoint_network_errno {
    net_endpoint_network_errno_none,
    net_endpoint_network_errno_remote_closed,
    net_endpoint_network_errno_dns_error,
    net_endpoint_network_errno_connect_error,
    net_endpoint_network_errno_network_error,
    net_endpoint_network_errno_user_closed,
    net_endpoint_network_errno_logic,
} net_endpoint_network_errno_t;

typedef enum net_endpoint_error_source {
    net_endpoint_error_source_network,
    net_endpoint_error_source_protocol,
    net_endpoint_error_source_user,
} net_endpoint_error_source_t;

NET_END_DECL

#endif
