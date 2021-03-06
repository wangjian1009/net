#ifndef NET_DNS_MANAGE_I_H_INCLEDED
#define NET_DNS_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_dns_manage.h"

NET_BEGIN_DECL

typedef struct net_dns_query_ex * net_dns_query_ex_t;
typedef struct net_dns_entry_alias * net_dns_entry_alias_t;
typedef struct net_dns_ns_cli_protocol * net_dns_ns_cli_protocol_t;
typedef struct net_dns_ns_cli_endpoint * net_dns_ns_cli_endpoint_t;
typedef struct net_dns_ns_parser * net_dns_ns_parser_t;
typedef struct net_dns_scope_source * net_dns_scope_source_t;

typedef TAILQ_HEAD(net_dns_entry_list, net_dns_entry) net_dns_entry_list_t;
typedef TAILQ_HEAD(net_dns_entry_item_list, net_dns_entry_item) net_dns_entry_item_list_t;
typedef TAILQ_HEAD(net_dns_entry_alias_list, net_dns_entry_alias) net_dns_entry_alias_list_t;
typedef TAILQ_HEAD(net_dns_source_list, net_dns_source) net_dns_source_list_t;
typedef TAILQ_HEAD(net_dns_task_list, net_dns_task) net_dns_task_list_t;
typedef TAILQ_HEAD(net_dns_task_step_list, net_dns_task_step) net_dns_task_step_list_t;
typedef TAILQ_HEAD(net_dns_task_ctx_list, net_dns_task_ctx) net_dns_task_ctx_list_t;
typedef TAILQ_HEAD(net_dns_task_builder_list, net_dns_task_builder) net_dns_task_builder_list_t;
typedef TAILQ_HEAD(net_dns_task_monitor_list, net_dns_task_monitor) net_dns_task_monitor_list_t;
typedef TAILQ_HEAD(net_dns_query_ex_list, net_dns_query_ex) net_dns_query_ex_list_t;
typedef TAILQ_HEAD(net_dns_scope_source_list, net_dns_scope_source) net_dns_scope_source_list_t;

struct net_dns_visit_node {
    net_dns_entry_t m_entry;
    net_dns_entry_alias_t m_child;
};

struct net_dns_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    uint32_t m_cfg_ttl_s;
    net_schedule_t m_schedule;
    net_dns_ns_cli_protocol_t m_protocol_dns_ns_cli;
    net_dns_source_list_t m_sources;
    net_dns_task_monitor_list_t m_monitors;

    net_dns_item_select_policy_t m_default_item_select_policy;
    
    net_dns_query_ex_list_t m_to_notify_querys;

    struct cpe_hash_table m_scopes;
    
    struct cpe_hash_table m_entries;
    struct cpe_hash_table m_items_by_ip;
    net_dns_entry_list_t m_free_entries;

    uint32_t m_visit_id_max;
    uint32_t m_visit_id_current;
    struct net_dns_visit_node m_visit_stack[32];

    net_timer_t m_delay_process;
    
    uint32_t m_task_ctx_capacity;
    struct cpe_hash_table m_tasks;
    net_dns_task_list_t m_runing_tasks;
    net_dns_task_list_t m_complete_tasks;

    net_dns_task_builder_t m_builder_default;
    net_dns_task_builder_t m_builder_internal;
    net_dns_task_builder_list_t m_builders;

    struct mem_buffer m_data_buffer;

    net_dns_task_list_t m_free_tasks;
    net_dns_task_step_list_t m_free_task_steps;
    net_dns_task_ctx_list_t m_free_task_ctxs;
    net_dns_entry_item_list_t m_free_entry_items;
    net_dns_entry_alias_list_t m_free_entry_aliass;
    net_dns_scope_source_list_t m_free_scope_sources;
};

mem_buffer_t net_dns_manage_tmp_buffer(net_dns_manage_t manage);
int net_dns_manage_active_delay_process(net_dns_manage_t manage);

NET_END_DECL

#endif
