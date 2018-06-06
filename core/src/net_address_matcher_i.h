#ifndef NET_ADDRESS_MATCHER_I_H_INCLEDED
#define NET_ADDRESS_MATCHER_I_H_INCLEDED
#include "net_address_matcher.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_address_matcher {
    net_schedule_t m_schedule;
    net_ipset_t m_ipset_ipv4;
    net_ipset_t m_ipset_ipv6;
    net_address_rule_list_t m_rules;
};

NET_END_DECL

#endif
