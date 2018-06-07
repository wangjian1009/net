#include "cpe/pal/pal_strings.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_dgram.h"
#include "net_dns_manage_i.h"
#include "net_dns_query_ex_i.h"
#include "net_dns_server_i.h"

static void net_dns_manage_fini(void * ctx);
static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_dns_mode_t mode)
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
    manage->m_mode = mode;
    TAILQ_INIT(&manage->m_servers);

    /* manage->m_dgram = net_dgram_create(schedule, NULL, net_dns_dgram_process, manage); */
    /* if (manage->m_dgram == NULL) { */
    /*     CPE_ERROR(em, "manage: create dgram fail!"); */
    /*     mem_free(alloc, manage); */
    /*     return NULL; */
    /* } */

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

    while(!TAILQ_EMPTY(&manage->m_servers)) {
        net_dns_server_free(TAILQ_FIRST(&manage->m_servers));
    }

    net_dgram_free(manage->m_dgram);
    manage->m_dgram = NULL;
    
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
