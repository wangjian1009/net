#ifndef NET_IPSET_H_INCLEDED
#define NET_IPSET_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

net_ipset_t net_ipset_create(net_schedule_t schedule);
void net_ipset_free(net_ipset_t ipset);

uint8_t net_ipset_is_empty(net_ipset_t set);
uint8_t net_ipset_is_equal(net_ipset_t set1, net_ipset_t set2);
size_t net_ipset_memory_size(net_ipset_t set);

/*ipv4*/
uint8_t net_ipset_ipv4_add(net_ipset_t set, net_address_data_ipv4_t elem);
uint8_t net_ipset_ipv4_add_network(net_ipset_t set, net_address_data_ipv4_t elem, uint8_t cidr_prefix);
uint8_t net_ipset_ipv4_remove(net_ipset_t set, net_address_data_ipv4_t elem);
uint8_t net_ipset_ipv4_remove_network(net_ipset_t set, net_address_data_ipv4_t elem, uint8_t cidr_prefix);
uint8_t net_ipset_contains_ipv4(net_ipset_t set, net_address_data_ipv4_t elem);

/*ipv6*/
uint8_t net_ipset_ipv6_add(net_ipset_t set, net_address_data_ipv6_t elem);
uint8_t net_ipset_ipv6_add_network(net_ipset_t set, net_address_data_ipv6_t elem, uint8_t cidr_prefix);
uint8_t net_ipset_ipv6_remove(net_ipset_t set, net_address_data_ipv6_t elem);
uint8_t net_ipset_ipv6_remove_network(net_ipset_t set, net_address_data_ipv6_t elem, uint8_t cidr_prefix);
uint8_t net_ipset_contains_ipv6(net_ipset_t set, net_address_data_ipv6_t elem);

/*uniip*/
uint8_t net_ipset_ip_add(net_ipset_t set, net_address_t address);
uint8_t net_ipset_ip_add_network(net_ipset_t set, net_address_t address, uint8_t cidr_prefix);
uint8_t net_ipset_ip_remove(net_ipset_t set, net_address_t address);
uint8_t net_ipset_ip_remove_network(net_ipset_t set, net_address_t address, uint8_t cidr_prefix);
uint8_t net_ipset_contains_ip(net_ipset_t set, net_address_t address);

NET_END_DECL

#endif
