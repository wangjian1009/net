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
typedef struct net_endpoint_next * net_endpoint_next_t;

typedef TAILQ_HEAD(net_local_ip_stack_monitor_list, net_local_ip_stack_monitor) net_local_ip_stack_monitor_list_t;
typedef TAILQ_HEAD(net_driver_list, net_driver) net_driver_list_t;
typedef TAILQ_HEAD(net_mem_group_list, net_mem_group) net_mem_group_list_t;
typedef TAILQ_HEAD(net_mem_block_list, net_mem_block) net_mem_block_list_t;
typedef TAILQ_HEAD(net_acceptor_list, net_acceptor) net_acceptor_list_t;
typedef TAILQ_HEAD(net_protocol_list, net_protocol) net_protocol_list_t;
typedef TAILQ_HEAD(net_endpoint_list, net_endpoint) net_endpoint_list_t;
typedef TAILQ_HEAD(net_endpoint_next_list, net_endpoint_next) net_endpoint_next_list_t;
typedef TAILQ_HEAD(net_endpoint_monitor_list, net_endpoint_monitor) net_endpoint_monitor_list_t;
typedef TAILQ_HEAD(net_dgram_list, net_dgram) net_dgram_list_t;
typedef TAILQ_HEAD(net_watcher_list, net_watcher) net_watcher_list_t;
typedef TAILQ_HEAD(net_timer_list, net_timer) net_timer_list_t;
typedef TAILQ_HEAD(net_address_rule_list, net_address_rule) net_address_rule_list_t;
typedef TAILQ_HEAD(net_address_list, net_address_in_cache) net_address_list_t;
typedef TAILQ_HEAD(net_dns_query_list, net_dns_query) net_dns_query_list_t;
typedef TAILQ_HEAD(net_debug_setup_list, net_debug_setup) net_debug_setup_list_t;
typedef TAILQ_HEAD(net_debug_condition_list, net_debug_condition) net_debug_condition_list_t;

struct net_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;

    net_timer_t m_delay_processor;

    net_debug_setup_list_t m_debug_setups;

    net_local_ip_stack_t m_local_ip_stack;
    net_local_ip_stack_monitor_list_t m_local_ip_stack_monitors;

    net_address_rule_t m_domain_address_rule;
    
    /*dns resolver*/
    void * m_dns_resolver_ctx;
    void (*m_dns_resolver_ctx_fini_fun)(void * ctx);
    net_schedule_dns_local_query_fun_t m_dns_local_query;
    uint16_t m_dns_query_capacity;
    net_schedule_dns_query_init_fun_t m_dns_query_init_fun;
    net_schedule_dns_query_fini_fun_t m_dns_query_fini_fun;
    uint32_t m_dns_max_query_id;
    struct cpe_hash_table m_dns_querys;

    /**/
    uint16_t m_endpoint_protocol_capacity;

    net_protocol_t m_noop_protocol;

    net_protocol_list_t m_protocols;
    net_driver_list_t m_drivers;

    uint32_t m_endpoint_max_id;
    struct cpe_hash_table m_endpoints;
    net_mem_group_t m_dft_mem_group;

    net_local_ip_stack_monitor_list_t m_free_local_ip_stack_monitors;
    net_address_list_t m_free_addresses;
    net_dns_query_list_t m_free_dns_querys;
    net_endpoint_monitor_list_t m_free_endpoint_monitors;
    net_endpoint_next_list_t m_free_endpoint_nexts;
    net_mem_group_list_t m_free_mem_groups;
    net_mem_block_list_t m_free_mem_blocks;

    struct mem_buffer m_tmp_buffer;
};

void net_schedule_start_delay_process(net_schedule_t schedule);

NET_END_DECL

#endif
