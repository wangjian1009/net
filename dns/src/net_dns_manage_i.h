#ifndef NET_DNS_MANAGE_I_H_INCLEDED
#define NET_DNS_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_dns_manage.h"

NET_BEGIN_DECL

typedef struct net_dns_query_ex * net_dns_query_ex_t;
typedef struct net_dns_task_ctx * net_dns_task_ctx_t;

typedef TAILQ_HEAD(net_dns_entry_list, net_dns_entry) net_dns_entry_list_t;
typedef TAILQ_HEAD(net_dns_source_list, net_dns_source) net_dns_source_list_t;
typedef TAILQ_HEAD(net_dns_task_list, net_dns_task) net_dns_task_list_t;
typedef TAILQ_HEAD(net_dns_task_step_list, net_dns_task_step) net_dns_task_step_list_t;
typedef TAILQ_HEAD(net_dns_task_ctx_list, net_dns_task_ctx) net_dns_task_ctx_list_t;
typedef TAILQ_HEAD(net_dns_task_builder_list, net_dns_task_builder) net_dns_task_builder_list_t;
typedef TAILQ_HEAD(net_dns_query_ex_list, net_dns_query_ex) net_dns_query_ex_list_t;

struct net_dns_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_schedule;
    net_driver_t m_driver;
    net_dns_mode_t m_mode;
    net_dns_source_list_t m_sources;
    net_dgram_t m_dgram;

    net_dns_query_ex_list_t m_to_notify_querys;
    
    struct cpe_hash_table m_entries;
    net_dns_entry_list_t m_free_entries;

    uint32_t m_task_ctx_capacity;
    
    uint32_t m_max_task_id;
    struct cpe_hash_table m_tasks;

    net_dns_task_builder_t m_builder_default;
    net_dns_task_builder_t m_builder_internal;
    net_dns_task_builder_list_t m_builders;

    net_dns_task_list_t m_free_tasks;
    net_dns_task_step_list_t m_free_task_steps;
    net_dns_task_ctx_list_t m_free_task_ctxs;
};

NET_END_DECL

#endif
