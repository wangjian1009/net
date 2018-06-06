#ifndef NET_IPSET_I_H_INCLEDED
#define NET_IPSET_I_H_INCLEDED
#include "net_ipset.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

typedef uint32_t net_ipset_node_id;
typedef uint32_t net_ipset_variable;
typedef uint32_t net_ipset_value;

typedef enum net_ipset_node_type {
    NET_IPSET_NONTERMINAL_NODE = 0,
    NET_IPSET_TERMINAL_NODE = 1
} net_ipset_node_type_t;

typedef struct net_ipset_node_cache * net_ipset_node_cache_t;
typedef struct net_ipset_node * net_ipset_node_t;

typedef uint8_t (*net_ipset_assignment_func_t)(const void *user_data, net_ipset_variable variable);

struct net_ipset {
    net_schedule_t m_schedule;
    net_ipset_node_cache_t m_cache;
    net_ipset_node_id m_set_bdd;
};

NET_END_DECL

#endif
