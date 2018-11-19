#ifndef NET_ADDRESS_MATCHER_H_INCLEDED
#define NET_ADDRESS_MATCHER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_address_matcher_t net_address_matcher_create(net_schedule_t schedule);
void net_address_matcher_free(net_address_matcher_t address_matcher);

uint8_t net_address_matcher_match(net_address_matcher_t address_matcher, net_address_t address);

int net_address_matcher_add(net_address_matcher_t matcher, net_address_t address);
int net_address_matcher_add_network(net_address_matcher_t matcher, net_address_t address, uint8_t cidr);
net_address_rule_t net_address_matcher_add_rule(net_address_matcher_t matcher, const char * rule);
net_address_rule_t net_address_rule_lookup(net_address_matcher_t matcher, const char * address);

int net_address_matcher_add_by_string(net_address_matcher_t matcher, const char * def);
    
NET_END_DECL

#endif
