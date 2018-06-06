#ifndef NET_IPSET_NODE_H_INCLEDED
#define NET_IPSET_NODE_H_INCLEDED
#include "net_ipset_i.h"

NET_BEGIN_DECL

struct net_ipset_node {
    net_schedule_t m_schedule;
    uint32_t m_refcount;
    net_ipset_variable m_variable;
    net_ipset_node_id m_low;
    net_ipset_node_id m_high;
    struct cpe_hash_entry m_hh;
};

#define net_ipset_node_get_type(node_id)  ((node_id) & 0x01)

#define net_ipset_terminal_value(node_id)  ((node_id) >> 1)
#define net_ipset_terminal_node_id(value) (((value) << 1) | NET_IPSET_TERMINAL_NODE)

#define net_ipset_nonterminal_value(node_id)  ((node_id) >> 1)
#define net_ipset_nonterminal_node_id(value) (((value) << 1) | NET_IPSET_NONTERMINAL_NODE)

uint32_t net_ipset_node_hash(void * user_data, const void * data);
uint8_t net_ipset_node_eq(void * user_data, const void * l, const void * r);

#define NET_IPSET_NODE_ID_FORMAT  "%s%u"
#define NET_IPSET_NODE_ID_VALUES(node_id) \
    (net_ipset_node_get_type((node_id)) == NET_IPSET_NONTERMINAL_NODE? "s": ""), \
    ((node_id) >> 1)

NET_END_DECL

#endif
