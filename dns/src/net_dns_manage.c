#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_timer.h"
#include "net_dns_query.h"
#include "net_dns_manage_i.h"
#include "net_dns_dgram_receiver_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_source_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_step_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_task_builder_i.h"

static void net_dns_manage_fini(void * ctx);
static void net_dns_manage_do_delay_process(net_timer_t timer, void * input_ctx);
static void net_dns_manage_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
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
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_mode = net_dns_ipv4_first;
    manage->m_task_ctx_capacity = 0;
    manage->m_builder_internal = NULL;
    manage->m_builder_default = NULL;
    manage->m_delay_process = NULL;
    manage->m_default_item_select_policy = net_dns_item_select_policy_first;
    
    TAILQ_INIT(&manage->m_sources);
    TAILQ_INIT(&manage->m_to_notify_querys);
    TAILQ_INIT(&manage->m_runing_tasks);
    TAILQ_INIT(&manage->m_complete_tasks);
    TAILQ_INIT(&manage->m_free_entries);
    TAILQ_INIT(&manage->m_free_tasks);
    TAILQ_INIT(&manage->m_free_task_steps);
    TAILQ_INIT(&manage->m_free_task_ctxs);
    TAILQ_INIT(&manage->m_free_entry_items);
    TAILQ_INIT(&manage->m_builders);

    if (cpe_hash_table_init(
            &manage->m_dgram_receivers,
            alloc,
            (cpe_hash_fun_t) net_dns_dgram_receiver_hash,
            (cpe_hash_eq_t) net_dns_dgram_receiver_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_dgram_receiver, m_hh),
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
        cpe_hash_table_fini(&manage->m_dgram_receivers);
        mem_free(alloc, manage);
        return NULL;
    }

    manage->m_builder_internal = net_dns_task_builder_internal_create(manage);
    if (manage->m_builder_internal == NULL) {
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_dgram_receivers);
        mem_free(alloc, manage);
        return NULL;
    }
    manage->m_builder_default = manage->m_builder_internal;
    
    manage->m_dgram = net_dgram_create(driver, NULL, net_dns_manage_dgram_process, manage);
    if (manage->m_dgram == NULL) {
        CPE_ERROR(em, "dns-cli: create dgram fail!");
        net_dns_task_builder_free(manage->m_builder_internal);
        cpe_hash_table_fini(&manage->m_entries);
        cpe_hash_table_fini(&manage->m_dgram_receivers);
        mem_free(alloc, manage);
        return NULL;
    }

    mem_buffer_init(&manage->m_data_buffer, alloc);

    net_schedule_set_dns_resolver(
        schedule,
        manage,
        net_dns_manage_fini,
        sizeof(struct net_dns_query_ex),
        net_dns_query_ex_init,
        net_dns_query_ex_fini);

    return manage;
}

void net_dns_manage_free(net_dns_manage_t manage) {
    if (net_schedule_dns_resolver(manage->m_schedule) == manage) {
        net_schedule_set_dns_resolver(manage->m_schedule, NULL, NULL, 0, NULL, NULL);
    }

    while(!TAILQ_EMPTY(&manage->m_runing_tasks)) {
        net_dns_task_free(TAILQ_FIRST(&manage->m_runing_tasks));
    }
    
    while(!TAILQ_EMPTY(&manage->m_complete_tasks)) {
        net_dns_task_free(TAILQ_FIRST(&manage->m_complete_tasks));
    }
    
    net_dns_entry_free_all(manage);
    cpe_hash_table_fini(&manage->m_entries);
    
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
    
    if (manage->m_dgram) {
        net_dgram_free(manage->m_dgram);
        manage->m_dgram = NULL;
    }

    net_dns_dgram_receiver_free_all(manage);
    cpe_hash_table_fini(&manage->m_dgram_receivers);

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
    
    mem_buffer_clear(&manage->m_data_buffer);
    
    mem_free(manage->m_alloc, manage);
}

static void net_dns_manage_fini(void * ctx) {
    //net_dns_manage_t manage = ctx;
}

net_dns_mode_t net_dns_manage_mode(net_dns_manage_t manage) {
    return manage->m_mode;
}

uint8_t net_dns_manage_debug(net_dns_manage_t manage) {
    return manage->m_debug;
}

void net_dns_manage_set_debug(net_dns_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
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

int net_dns_manage_dgram_send(net_dns_manage_t manage, net_address_t target, void const * data, size_t data_size) {
    return net_dgram_send(manage->m_dgram, target, data, data_size);
}

static void net_dns_manage_do_delay_process(net_timer_t timer, void * input_ctx) {
    net_dns_manage_t manage = input_ctx;

    manage->m_delay_process = NULL;

    while(!TAILQ_EMPTY(&manage->m_complete_tasks)) {
        net_dns_task_t task = TAILQ_FIRST(&manage->m_complete_tasks);

        while(!TAILQ_EMPTY(&task->m_querys)) {
            net_dns_query_ex_t query_ex = TAILQ_FIRST(&task->m_querys);
            query_ex->m_entry = task->m_entry;
            net_dns_query_ex_set_task(query_ex, NULL);
        }

        net_dns_task_free(task);
    }

    while(!TAILQ_EMPTY(&manage->m_to_notify_querys)) {
        net_dns_query_ex_t query_ex = TAILQ_FIRST(&manage->m_to_notify_querys);
        net_dns_query_t query = net_dns_query_from_data(query_ex);
        net_address_t address = NULL;
        struct net_address_it address_it;
        net_address_it_init(&address_it);

        if (query_ex->m_entry) {
            net_dns_entry_item_t item = 
                net_dns_entry_select_item(query_ex->m_entry, manage->m_default_item_select_policy);
            if (item == NULL) {
                CPE_ERROR(manage->m_em, "dns-cli: query %s: no item!", query_ex->m_entry->m_hostname);
            }
            else {
                address = item->m_address;
            }

            net_dns_entry_addresses(query_ex->m_entry, &address_it);
        }

        net_dns_query_notify_result_and_free(query, address, &address_it);
    }
}

static void net_dns_manage_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source)
{
    net_dns_manage_t manage = ctx;

    net_dns_dgram_receiver_t receiver = net_dns_dgram_receiver_find_by_address(manage, source);
    if (receiver == NULL) {
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: no dgram receiver process data from %s, ignore",
                net_address_dump(net_dns_manage_tmp_buffer(manage), source));
        }
        return;
    }

    receiver->m_process_fun(receiver->m_process_ctx, data, data_size);
}
