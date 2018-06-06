#ifndef NET_SCHEDULE_I_H_INCLEDED
#define NET_SCHEDULE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"

NET_BEGIN_DECL

typedef struct net_address_in_cache * net_address_in_cache_t;

typedef TAILQ_HEAD(net_driver_list, net_driver) net_driver_list_t;
typedef TAILQ_HEAD(net_protocol_list, net_protocol) net_protocol_list_t;
typedef TAILQ_HEAD(net_router_list, net_router) net_router_list_t;
typedef TAILQ_HEAD(net_endpoint_list, net_endpoint) net_endpoint_list_t;
typedef TAILQ_HEAD(net_dgram_list, net_dgram) net_dgram_list_t;
typedef TAILQ_HEAD(net_timer_list, net_timer) net_timer_list_t;
typedef TAILQ_HEAD(net_address_rule_list, net_address_rule) net_address_rule_list_t;
typedef TAILQ_HEAD(net_address_list, net_address_in_cache) net_address_list_t;
typedef TAILQ_HEAD(net_link_list, net_link) net_link_list_t;
typedef TAILQ_HEAD(net_dns_server_list, net_dns_server) net_dns_server_list_t;
typedef TAILQ_HEAD(net_dns_query_list, net_dns_query) net_dns_query_list_t;

struct net_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;

    net_data_monitor_fun_t m_data_monitor_fun;
    void * m_data_monitor_ctx;

    net_dns_resolver_t m_dns_resolver;
    
    uint16_t m_endpoint_protocol_capacity;

    net_protocol_t m_direct_protocol;
    net_driver_t m_direct_driver;
    net_address_matcher_t m_direct_matcher_white;
    net_address_matcher_t m_direct_matcher_black;
    
    net_protocol_list_t m_protocols;
    net_driver_list_t m_drivers;
    net_router_list_t m_routers;
    net_link_list_t m_links;

    uint32_t m_endpoint_max_id;
    struct cpe_hash_table m_endpoints;
    ringbuffer_t m_endpoint_buf;
    ringbuffer_block_t m_endpoint_tb;

    net_address_list_t m_free_addresses;
    net_link_list_t m_free_links;
    net_dns_query_list_t m_free_dns_querys;
    
    struct mem_buffer m_tmp_buffer;
};

NET_END_DECL

#endif
