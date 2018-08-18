#ifndef NET_DEBUG_SETUP_I_H_INCLEDED
#define NET_DEBUG_SETUP_I_H_INCLEDED
#include "net_debug_setup.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_debug_setup {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_debug_setup) m_next_for_schedule;
    uint8_t m_protocol_debug;
    uint8_t m_driver_debug;
    net_debug_condition_list_t m_conditions;
};

uint8_t net_debug_setup_check(net_debug_setup_t setup, net_endpoint_t endpoint);
    
NET_END_DECL

#endif
