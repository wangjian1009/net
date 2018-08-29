#ifndef NET_SYSTEM_H_INCLEDED
#define NET_SYSTEM_H_INCLEDED
#include "net_common.h"

NET_BEGIN_DECL

typedef struct net_schedule * net_schedule_t;
typedef struct net_driver * net_driver_t;
typedef struct net_protocol * net_protocol_t;
typedef struct net_link * net_link_t;
typedef struct net_timer * net_timer_t;
typedef struct net_endpoint * net_endpoint_t;
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
} net_address_type_t;

typedef enum net_endpoint_buf_type {
    net_ep_buf_read,
    net_ep_buf_forward,
    net_ep_buf_write,
    net_ep_buf_tmp,
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

typedef enum net_endpoint_type {
    net_endpoint_inbound,
    net_endpoint_outbound,
} net_endpoint_type_t;

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

/*acceptor*/
typedef int (*net_acceptor_on_new_endpoint_fun_t)(void * ctx, net_endpoint_t endpoint);

/*monitor*/
typedef void (*net_data_monitor_fun_t) (
    void * ctx, net_endpoint_t endpoint, net_data_direct_t direct, uint32_t sz);

/*dns*/
typedef void (*net_dns_query_callback_fun_t)(void * ctx, net_address_t main_address, net_address_it_t all_address);
typedef void (*net_dns_query_ctx_free_fun_t)(void * ctx);

NET_END_DECL

#endif
