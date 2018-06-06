#include "net_address.h"
#include "net_ipset_i.h"
#include "net_ipset_node.h"
#include "net_ipset_node_cache.h"

net_ipset_t net_ipset_create(net_schedule_t schedule) {
    net_ipset_t ipset = mem_alloc(schedule->m_alloc, sizeof(struct net_ipset));
    if (ipset == NULL) {
        CPE_ERROR(schedule->m_em, "net_ipset_create: alloc fail!");
        return NULL;
    }
    
    return ipset;
}

void net_ipset_free(net_ipset_t ipset) {
}

uint8_t net_ipset_is_empty(net_ipset_t set) {
    return set->m_set_bdd == net_ipset_terminal_node_id(0) ? 1 : 0;
}

uint8_t net_ipset_is_equal(net_ipset_t set1, net_ipset_t set2) {
    return net_ipset_node_cache_nodes_equal(set1->m_cache, set1->m_set_bdd, set2->m_cache, set2->m_set_bdd);
}

size_t net_ipset_memory_size(net_ipset_t set) {
    return net_ipset_node_memory_size(set->m_cache, set->m_set_bdd);
}

uint8_t net_ipset_ip_add(net_ipset_t set, net_address_t address) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_ipv4_add(set, (net_address_data_ipv4_t)(net_address_data(address)));
    case net_address_ipv6:
        return net_ipset_ipv6_add(set, (net_address_data_ipv6_t)(net_address_data(address)));
    case net_address_domain:
        return 0;
    }
}

uint8_t net_ipset_ip_add_network(net_ipset_t set, net_address_t address, uint8_t cidr_prefix) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_ipv4_add_network(set, (net_address_data_ipv4_t)(net_address_data(address)), cidr_prefix);
    case net_address_ipv6:
        return net_ipset_ipv6_add_network(set, (net_address_data_ipv6_t)(net_address_data(address)), cidr_prefix);
    case net_address_domain:
        return 0;
    }
}

uint8_t net_ipset_ip_remove(net_ipset_t set, net_address_t address) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_ipv4_remove(set, (net_address_data_ipv4_t)(net_address_data(address)));
    case net_address_ipv6:
        return net_ipset_ipv6_remove(set, (net_address_data_ipv6_t)(net_address_data(address)));
    case net_address_domain:
        return 0;
    }
}

uint8_t net_ipset_ip_remove_network(net_ipset_t set, net_address_t address, uint8_t cidr_prefix) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_ipv4_remove_network(set, (net_address_data_ipv4_t)net_address_data(address), cidr_prefix);
    case net_address_ipv6:
        return net_ipset_ipv6_remove_network(set, (net_address_data_ipv6_t)net_address_data(address), cidr_prefix);
    case net_address_domain:
        return 0;
    }
}

uint8_t net_ipset_contains_ip(net_ipset_t set, net_address_t address) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_contains_ipv4(set, (net_address_data_ipv4_t)net_address_data(address));
    case net_address_ipv6:
        return net_ipset_contains_ipv6(set, (net_address_data_ipv6_t)net_address_data(address));
    case net_address_domain:
        return 0;
    }
}
