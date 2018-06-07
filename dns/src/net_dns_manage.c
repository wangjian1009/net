#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_dgram.h"
#include "net_dns_manage_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_server_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_task_i.h"

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
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_mode = net_dns_ipv4_first;
    TAILQ_INIT(&manage->m_servers);
    TAILQ_INIT(&manage->m_to_notify_querys);
    TAILQ_INIT(&manage->m_free_entries);
    TAILQ_INIT(&manage->m_free_tasks);

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
    
    manage->m_dgram = net_dgram_create(driver, NULL, net_dns_dgram_process, manage);
    if (manage->m_dgram == NULL) {
        CPE_ERROR(em, "dns: create dgram fail!");
        cpe_hash_table_fini(&manage->m_tasks);
        cpe_hash_table_fini(&manage->m_entries);
        mem_free(alloc, manage);
        return NULL;
    }

    net_schedule_set_dns_resolver(
        schedule,
        manage,
        net_dns_manage_fini,
        0,
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
    
    while(!TAILQ_EMPTY(&manage->m_servers)) {
        net_dns_server_free(TAILQ_FIRST(&manage->m_servers));
    }

    if (manage->m_dgram) {
        net_dgram_free(manage->m_dgram);
        manage->m_dgram = NULL;
    }

    while(!TAILQ_EMPTY(&manage->m_free_entries)) {
        net_dns_entry_real_free(manage, TAILQ_FIRST(&manage->m_free_entries));
    }

    while(!TAILQ_EMPTY(&manage->m_free_tasks)) {
        net_dns_task_real_free(TAILQ_FIRST(&manage->m_free_tasks));
    }
    
    mem_free(manage->m_alloc, manage);
}

static void net_dns_manage_fini(void * ctx) {
    net_dns_manage_t manage = ctx;
}

net_dns_mode_t net_dns_manage_mode(net_dns_manage_t manage) {
    return manage->m_mode;
}

static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source)
{
    
}
