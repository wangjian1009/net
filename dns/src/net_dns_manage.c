#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_dns_query.h"
#include "net_dns_manage_i.h"
#include "net_dns_scope_i.h"
#include "net_dns_scope_source_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_source_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_entry_alias_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_step_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_task_builder_i.h"
#include "net_dns_task_monitor_i.h"
#include "net_dns_ns_cli_protocol_i.h"

static void net_dns_manage_fini(void * ctx);
static void net_dns_manage_do_delay_process(net_timer_t timer, void * input_ctx);
static int net_dns_manage_local_query(void * manage, net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive);

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule)
{
    net_dns_manage_t manage = mem_alloc(alloc, sizeof(struct net_dns_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "dns-cli: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_cfg_ttl_s = 3 * 60;
    manage->m_schedule = schedule;
    manage->m_task_ctx_capacity = 0;
    manage->m_builder_internal = NULL;
    manage->m_builder_default = NULL;
    manage->m_delay_process = NULL;
    manage->m_default_item_select_policy = net_dns_item_select_policy_first;
    
    TAILQ_INIT(&manage->m_sources);
    TAILQ_INIT(&manage->m_to_notify_querys);
    TAILQ_INIT(&manage->m_runing_tasks);
    TAILQ_INIT(&manage->m_complete_tasks);
    TAILQ_INIT(&manage->m_monitors);
    TAILQ_INIT(&manage->m_free_entries);
    TAILQ_INIT(&manage->m_free_tasks);
    TAILQ_INIT(&manage->m_free_task_steps);
    TAILQ_INIT(&manage->m_free_task_ctxs);
    TAILQ_INIT(&manage->m_free_entry_items);
    TAILQ_INIT(&manage->m_free_entry_aliass);
    TAILQ_INIT(&manage->m_free_scope_sources);
    TAILQ_INIT(&manage->m_builders);

    if (cpe_hash_table_init(
            &manage->m_scopes,
            alloc,
            (cpe_hash_fun_t) net_dns_scope_hash,
            (cpe_hash_eq_t) net_dns_scope_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_scope, m_hh),
            -1) != 0)
    {
        mem_free(alloc, manage);
        return NULL;
    }
    
    if (cpe_hash_table_init(
            &manage->m_entries,
            alloc,
            (cpe_hash_fun_t) net_dns_entry_hash,
            (cpe_hash_eq_t) net_dns_entry_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_entry, m_hh),
            -1) != 0)
    {
        cpe_hash_table_fini(&manage->m_scopes);
        mem_free(alloc, manage);
        return NULL;
    }

    if (cpe_hash_table_init(
            &manage->m_tasks,
            alloc,
            (cpe_hash_fun_t) net_dns_task_hash,
            (cpe_hash_eq_t) net_dns_task_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_task, m_hh),
            -1) != 0)
    {
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_scopes);
        mem_free(alloc, manage);
        return NULL;
    }

    if (cpe_hash_table_init(
            &manage->m_items_by_ip,
            alloc,
            (cpe_hash_fun_t) net_dns_entry_item_hash_by_ip,
            (cpe_hash_eq_t) net_dns_entry_item_eq_by_ip,
            CPE_HASH_OBJ2ENTRY(net_dns_entry_item, m_hh_for_ip),
            -1) != 0)
    {
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_scopes);
        mem_free(alloc, manage);
        return NULL;
    }
    
    manage->m_protocol_dns_ns_cli = net_dns_ns_cli_protocol_create(manage);
    if (manage->m_protocol_dns_ns_cli == NULL) {
        cpe_hash_table_fini(&manage->m_items_by_ip);
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_scopes);
        mem_free(alloc, manage);
        return NULL;
    }
    
    manage->m_builder_internal = net_dns_task_builder_internal_create(manage);
    if (manage->m_builder_internal == NULL) {
        net_dns_ns_cli_protocol_free(manage->m_protocol_dns_ns_cli);
        cpe_hash_table_fini(&manage->m_items_by_ip);
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_scopes);
        mem_free(alloc, manage);
        return NULL;
    }
    manage->m_builder_default = manage->m_builder_internal;
    
    mem_buffer_init(&manage->m_data_buffer, alloc);

    net_schedule_set_dns_resolver(
        schedule,
        manage,
        net_dns_manage_fini,
        net_dns_manage_local_query,
        sizeof(struct net_dns_query_ex),
        net_dns_query_ex_init,
        net_dns_query_ex_fini);

    return manage;
}

void net_dns_manage_free(net_dns_manage_t manage) {
    if (net_schedule_dns_resolver(manage->m_schedule) == manage) {
        net_schedule_set_dns_resolver(manage->m_schedule, NULL, NULL, NULL, 0, NULL, NULL);
    }

    while(!TAILQ_EMPTY(&manage->m_runing_tasks)) {
        net_dns_task_free(TAILQ_FIRST(&manage->m_runing_tasks));
    }
    
    while(!TAILQ_EMPTY(&manage->m_complete_tasks)) {
        net_dns_task_free(TAILQ_FIRST(&manage->m_complete_tasks));
    }

    while(!TAILQ_EMPTY(&manage->m_monitors)) {
        net_dns_task_monitor_free(TAILQ_FIRST(&manage->m_monitors));
    }
    
    net_dns_entry_free_all(manage);
    net_dns_scope_free_all(manage);

    while(!TAILQ_EMPTY(&manage->m_sources)) {
        net_dns_source_free(TAILQ_FIRST(&manage->m_sources));
    }

    while(!TAILQ_EMPTY(&manage->m_builders)) {
        net_dns_task_builder_free(TAILQ_FIRST(&manage->m_builders));
    }
    assert(manage->m_builder_default == NULL);
    assert(manage->m_builder_internal == NULL);

    if (manage->m_delay_process) {
        net_timer_free(manage->m_delay_process);
        manage->m_delay_process = NULL;
    }

    if (manage->m_protocol_dns_ns_cli) {
        net_dns_ns_cli_protocol_free(manage->m_protocol_dns_ns_cli);
        manage->m_protocol_dns_ns_cli = NULL;
    }
    
    cpe_hash_table_fini(&manage->m_tasks);
    cpe_hash_table_fini(&manage->m_entries);
    cpe_hash_table_fini(&manage->m_scopes);

    while(!TAILQ_EMPTY(&manage->m_free_entries)) {
        net_dns_entry_real_free(TAILQ_FIRST(&manage->m_free_entries));
    }

    while(!TAILQ_EMPTY(&manage->m_free_tasks)) {
        net_dns_task_real_free(TAILQ_FIRST(&manage->m_free_tasks));
    }
    
    while(!TAILQ_EMPTY(&manage->m_free_task_steps)) {
        net_dns_task_step_real_free(TAILQ_FIRST(&manage->m_free_task_steps));
    }
    
    while(!TAILQ_EMPTY(&manage->m_free_task_ctxs)) {
        net_dns_task_ctx_real_free(TAILQ_FIRST(&manage->m_free_task_ctxs));
    }

    while(!TAILQ_EMPTY(&manage->m_free_entry_items)) {
        net_dns_entry_item_real_free(TAILQ_FIRST(&manage->m_free_entry_items));
    }

    while(!TAILQ_EMPTY(&manage->m_free_entry_aliass)) {
        net_dns_entry_alias_real_free(TAILQ_FIRST(&manage->m_free_entry_aliass));
    }

    while(!TAILQ_EMPTY(&manage->m_free_scope_sources)) {
        net_dns_scope_source_real_free(TAILQ_FIRST(&manage->m_free_scope_sources));
    }
    
    mem_buffer_clear(&manage->m_data_buffer);
    
    mem_free(manage->m_alloc, manage);
}

static void net_dns_manage_fini(void * ctx) {
    //net_dns_manage_t manage = ctx;
}

uint8_t net_dns_manage_debug(net_dns_manage_t manage) {
    return manage->m_debug;
}

void net_dns_manage_set_debug(net_dns_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}

void net_dns_manage_clear_cache(net_dns_manage_t manage) {
    struct cpe_hash_it entry_it;
    cpe_hash_it_init(&entry_it, &manage->m_entries);

    net_dns_entry_t entry;
    while((entry = cpe_hash_it_next(&entry_it))) {
        while(!TAILQ_EMPTY(&entry->m_items)) {
            net_dns_entry_item_free(TAILQ_FIRST(&entry->m_items));
        }
        entry->m_expire_time_s = 0;
    }
}

net_dns_task_builder_t net_dns_task_builder_internal(net_dns_manage_t manage) {
    return manage->m_builder_internal;
}

net_dns_task_builder_t net_dns_task_builder_default(net_dns_manage_t manage) {
    return manage->m_builder_default;
}

void net_dns_task_builder_set_default(net_dns_manage_t manage, net_dns_task_builder_t builder) {
    manage->m_builder_default = builder ? builder : manage->m_builder_internal;
}

mem_buffer_t net_dns_manage_tmp_buffer(net_dns_manage_t manage) {
    return net_schedule_tmp_buffer(manage->m_schedule);
}

int net_dns_manage_active_delay_process(net_dns_manage_t manage) {
    if (manage->m_delay_process == NULL) {
        manage->m_delay_process = net_timer_auto_create(manage->m_schedule, net_dns_manage_do_delay_process, manage);
        if (manage->m_delay_process == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: create delay process timer fail!");
            return -1;
        }
    }

    if (!net_timer_is_active(manage->m_delay_process)) {
        net_timer_active(manage->m_delay_process, 0);
    }

    return 0;
}

static int net_dns_manage_local_query(void * ctx, net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive) {
    net_dns_manage_t manage = ctx;

    assert(hostname);
    assert(net_address_type(hostname) == net_address_domain);

    net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
    if (entry == NULL) {
        net_address_it_init(resolved_it);
        return 0;
    }

    net_dns_entry_addresses(entry, resolved_it, recursive);
    return 0;
}

static void net_dns_manage_do_delay_process(net_timer_t timer, void * input_ctx) {
    net_dns_manage_t manage = input_ctx;

    while(!TAILQ_EMPTY(&manage->m_complete_tasks)) {
        net_dns_task_t task = TAILQ_FIRST(&manage->m_complete_tasks);

        while(!TAILQ_EMPTY(&task->m_querys)) {
            net_dns_query_ex_t query_ex = TAILQ_FIRST(&task->m_querys);
            net_dns_query_ex_set_task(query_ex, NULL);
        }

        net_dns_task_monitor_t monitor;
        TAILQ_FOREACH(monitor, &manage->m_monitors, m_next) {
            monitor->m_on_complete(monitor->m_ctx, task);
        }
        
        net_dns_task_free(task);
    }

    while(!TAILQ_EMPTY(&manage->m_to_notify_querys)) {
        net_dns_query_ex_t query_ex = TAILQ_FIRST(&manage->m_to_notify_querys);
        net_dns_query_t query = net_dns_query_from_data(query_ex);
        net_address_t address = NULL;
        struct net_address_it address_it;
        net_address_it_init(&address_it);

        if (query_ex->m_query_type == net_dns_query_domain) {
            assert(query_ex->m_address);

            address = net_dns_hostname_by_ip(manage, query_ex->m_address);
            net_dns_hostnames_by_ip(&address_it, manage, query_ex->m_address);
        }
        else {
            net_dns_entry_t entry = net_dns_entry_find(manage, query_ex->m_address);
            if (entry) {
                net_dns_entry_item_t item = 
                    net_dns_entry_select_item(entry, manage->m_default_item_select_policy, query_ex->m_query_type);
                if (item == NULL) {
                    if (manage->m_debug) {
                        CPE_INFO(
                            manage->m_em, "dns-cli: query %s %s: no item!",
                            net_address_dump(net_dns_manage_tmp_buffer(manage), query_ex->m_address),
                            net_dns_query_type_str(query_ex->m_query_type));
                    }
                }
                else {
                    address = item->m_address;
                }

                net_dns_entry_addresses(entry, &address_it, 1);
            }
            else {
                if (manage->m_debug) {
                    CPE_INFO(
                        manage->m_em, "dns-cli: query %s %s: no entry!",
                        net_address_dump(net_dns_manage_tmp_buffer(manage), query_ex->m_address),
                        net_dns_query_type_str(query_ex->m_query_type));
                }
            }
        }

        if (manage->m_debug) {
            char buf[128];
            cpe_str_dup(buf, sizeof(buf), net_address_dump(net_dns_manage_tmp_buffer(manage), query_ex->m_address));
            CPE_INFO(
                manage->m_em, "dns-cli: response: %s %s ==> %s",
                buf, net_dns_query_type_str(query_ex->m_query_type),
                address ? net_address_dump(net_dns_manage_tmp_buffer(manage), address) : "N/A");
        }

        CPE_ERROR(
            manage->m_em, "dns-cli: query %s(%p): notify!",
            net_address_dump(net_dns_manage_tmp_buffer(manage), query_ex->m_address), query_ex);
    
        net_dns_query_notify_result_and_free(query, address, &address_it);
    }
}
