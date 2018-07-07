#include "assert.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/ringbuffer.h"
#include "net_endpoint.h"
#include "net_schedule.h"
#include "net_address_matcher.h"
#include "net_router_schedule_i.h"
#include "net_router_i.h"

net_router_schedule_t
net_router_schedule_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t net_schedule) {
    net_router_schedule_t schedule;
    
    schedule = mem_alloc(alloc, sizeof(struct net_router_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "schedule: alloc fail!");
        return NULL;
    }

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = 0;
    schedule->m_net_schedule = net_schedule;
    schedule->m_direct_matcher_white = NULL;
    schedule->m_direct_matcher_black = NULL;

    TAILQ_INIT(&schedule->m_routers);

    return schedule;
}

void net_router_schedule_free(net_router_schedule_t schedule) {
    if (schedule->m_direct_matcher_white) {
        net_address_matcher_free(schedule->m_direct_matcher_white);
        schedule->m_direct_matcher_white = NULL;
    }
    
    if (schedule->m_direct_matcher_black) {
        net_address_matcher_free(schedule->m_direct_matcher_black);
        schedule->m_direct_matcher_black = NULL;
    }
    
    while(!TAILQ_EMPTY(&schedule->m_routers)) {
        net_router_free(TAILQ_FIRST(&schedule->m_routers));
    }
}

mem_allocrator_t net_router_schedule_allocrator(net_router_schedule_t schedule) {
    return schedule->m_alloc;
}

error_monitor_t net_router_schedule_em(net_router_schedule_t schedule) {
    return schedule->m_em;
}

mem_buffer_t net_router_schedule_tmp_buffer(net_router_schedule_t schedule) {
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}

net_address_matcher_t
net_router_schedule_direct_matcher_white(net_router_schedule_t schedule) {
    return schedule->m_direct_matcher_white;
}

net_address_matcher_t
net_router_schedule_direct_matcher_white_check_create(net_router_schedule_t schedule) {
    if (schedule->m_direct_matcher_white) {
        schedule->m_direct_matcher_white = net_address_matcher_create(schedule->m_net_schedule);
    }

    return schedule->m_direct_matcher_white;
}

net_address_matcher_t
net_router_schedule_direct_matcher_black(net_router_schedule_t schedule) {
    return schedule->m_direct_matcher_black;
}

net_address_matcher_t
net_router_schedule_direct_matcher_black_check_create(net_router_schedule_t schedule) {
    if (schedule->m_direct_matcher_black) {
        schedule->m_direct_matcher_black = net_address_matcher_create(schedule->m_net_schedule);
    }

    return schedule->m_direct_matcher_black;
}

uint8_t net_router_schedule_debug(net_router_schedule_t schedule) {
    return schedule->m_debug;
}

int net_router_schedule_auto_select(net_router_schedule_t schedule, net_endpoint_t endpoint, net_address_t target_addr, uint8_t is_own) {
    net_router_t router;

    TAILQ_FOREACH(router, &schedule->m_routers, m_next_for_schedule) {
        return net_router_link(router, endpoint, target_addr, is_own);
    }

    return net_endpoint_link_direct(endpoint, target_addr, is_own);
}

int net_router_schedule_do_connect(void * ctx, net_endpoint_t endpoint, net_address_t target, uint8_t is_own) {
    return 0;
}
