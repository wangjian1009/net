#ifndef NET_ROUTER_I_H_INCLEDED
#define NET_ROUTER_I_H_INCLEDED
#include "net_router.h"
#include "net_router_schedule_i.h"

NET_BEGIN_DECL

struct net_router {
    net_router_schedule_t m_schedule;
    TAILQ_ENTRY(net_router) m_next_for_schedule;
    net_address_t m_address;
    net_protocol_t m_protocol;
    net_driver_t m_driver;
    net_address_matcher_t m_matcher_white;
    net_address_matcher_t m_matcher_black;
};

NET_END_DECL

#endif
