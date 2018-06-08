#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_dgram.h"
#include "net_dns_manage_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_source_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_step_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_task_builder_i.h"

static void net_dns_manage_fini(void * ctx);
static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    net_dns_manage_t manage = mem_alloc(alloc, sizeof(struct net_dns_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "dns: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_mode = net_dns_ipv4_first;
    manage->m_max_task_id = 0;
    manage->m_task_ctx_capacity = 0;
    manage->m_builder_internal = NULL;
    manage->m_builder_default = NULL;
    TAILQ_INIT(&manage->m_sources);
    TAILQ_INIT(&manage->m_to_notify_querys);
    TAILQ_INIT(&manage->m_free_entries);
    TAILQ_INIT(&manage->m_free_tasks);
    TAILQ_INIT(&manage->m_free_task_steps);
    TAILQ_INIT(&manage->m_free_task_ctxs);
    TAILQ_INIT(&manage->m_builders);

    if (cpe_hash_table_init(
            &manage->m_entries,
            alloc,
            (cpe_hash_fun_t) net_dns_entry_hash,
            (cpe_hash_eq_t) net_dns_entry_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_entry, m_hh),
            -1) != 0)
    {
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
        mem_free(alloc, manage);
        return NULL;
    }

    manage->m_builder_internal = net_dns_task_builder_internal_create(manage);
    if (manage->m_builder_internal == NULL) {
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        mem_free(alloc, manage);
        return NULL;
    }
    manage->m_builder_default = manage->m_builder_internal;
    
    manage->m_dgram = net_dgram_create(driver, NULL, net_dns_dgram_process, manage);
    if (manage->m_dgram == NULL) {
        CPE_ERROR(em, "dns: create dgram fail!");
        net_dns_task_builder_free(manage->m_builder_internal);
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        mem_free(alloc, manage);
        return NULL;
    }

    net_schedule_set_dns_resolver(
        schedule,
        manage,
        net_dns_manage_fini,
        sizeof(struct net_dns_query_ex),
        net_dns_query_ex_start,
        net_dns_query_ex_cancel);
    
    return manage;
}

void net_dns_manage_free(net_dns_manage_t manage) {
    if (net_schedule_dns_resolver(manage->m_schedule) == manage) {
        net_schedule_set_dns_resolver(manage->m_schedule, NULL, NULL, 0, NULL, NULL);
    }

    net_dns_task_free_all(manage);
    net_dns_entry_free_all(manage);
    cpe_hash_table_fini(&manage->m_tasks);
    cpe_hash_table_fini(&manage->m_entries);
    
    while(!TAILQ_EMPTY(&manage->m_sources)) {
        net_dns_source_free(TAILQ_FIRST(&manage->m_sources));
    }

    if (manage->m_dgram) {
        net_dgram_free(manage->m_dgram);
        manage->m_dgram = NULL;
    }

    while(!TAILQ_EMPTY(&manage->m_builders)) {
        net_dns_task_builder_free(TAILQ_FIRST(&manage->m_builders));
    }
    assert(manage->m_builder_default == NULL);
    assert(manage->m_builder_internal == NULL);

    while(!TAILQ_EMPTY(&manage->m_free_entries)) {
        net_dns_entry_real_free(manage, TAILQ_FIRST(&manage->m_free_entries));
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
    
    mem_free(manage->m_alloc, manage);
}

static void net_dns_manage_fini(void * ctx) {
    net_dns_manage_t manage = ctx;
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

static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source)
{
    
}
