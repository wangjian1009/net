#ifndef NET_DEBUG_CONDITION_I_H_INCLEDED
#define NET_DEBUG_CONDITION_I_H_INCLEDED
#include "net_debug_condition.h"
#include "net_debug_setup_i.h"

NET_BEGIN_DECL

struct net_debug_condition {
    net_debug_setup_t m_setup;
    TAILQ_ENTRY(net_debug_condition) m_next;
    net_debug_condition_scope_t m_scope;
    net_debug_condition_type_t m_type;
    union {
        struct {
            net_address_t m_address;
        } m_address;
    };
};

uint8_t net_debug_condition_check(net_debug_condition_t condition, net_address_t address);

NET_END_DECL

#endif
