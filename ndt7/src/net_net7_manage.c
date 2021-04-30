#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_ndt7_manage_i.h"

net_ndt7_manage_t net_ndt7_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule)
{
    net_ndt7_manage_t manage = mem_alloc(alloc, sizeof(struct net_ndt7_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "ndt-cli: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;

    return manage;
}

void net_ndt7_manage_free(net_ndt7_manage_t manage) {
    mem_free(manage->m_alloc, manage);
}

uint8_t net_ndt7_manage_debug(net_ndt7_manage_t manage) {
    return manage->m_debug;
}

void net_ndt7_manage_set_debug(net_ndt7_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}
