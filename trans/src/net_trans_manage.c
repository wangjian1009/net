#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_trans_manage_i.h"
#include "net_trans_http_protocol_i.h"

net_trans_manage_t net_trans_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule)
{
    net_trans_manage_t manage = mem_alloc(alloc, sizeof(struct net_trans_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "trans-cli: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;

    manage->m_http_protocol = net_trans_http_protocol_create(manage);
    if (manage->m_http_protocol == NULL) {
        mem_free(alloc, manage);
        return NULL;
    }
    
    return manage;
}

void net_trans_manage_free(net_trans_manage_t manage) {
    if (manage->m_http_protocol) {
        net_trans_http_protocol_free(manage->m_http_protocol);
        manage->m_http_protocol = NULL;
    }
    
    mem_free(manage->m_alloc, manage);
}

uint8_t net_trans_manage_debug(net_trans_manage_t manage) {
    return manage->m_debug;
}

void net_trans_manage_set_debug(net_trans_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}
