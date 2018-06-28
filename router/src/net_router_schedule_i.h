#ifndef NET_SCHEDULE_I_H_INCLEDED
#define NET_SCHEDULE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_router_list, net_router) net_router_list_t;

struct net_router_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_net_schedule;

    net_address_matcher_t m_direct_matcher_white;
    net_address_matcher_t m_direct_matcher_black;
    
    net_router_list_t m_routers;
};

NET_END_DECL

#endif
