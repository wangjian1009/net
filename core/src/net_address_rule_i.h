#ifndef NET_ADDRESS_RULE_I_H_INCLEDED
#define NET_ADDRESS_RULE_I_H_INCLEDED
#include "pcre2.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_address_rule {
    TAILQ_ENTRY(net_address_rule) m_next;
    pcre2_code * m_pattern_re;
};

net_address_rule_t net_address_rule_create(net_schedule_t schedule, const char * rule);
void net_address_rule_free(net_schedule_t schedule, net_address_rule_t rule);

uint8_t net_address_rule_check(net_schedule_t schedule, net_address_rule_t rule, const char * address);

NET_END_DECL

#endif
