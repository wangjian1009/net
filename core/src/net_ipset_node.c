#include "cpe/utils/stream.h"
#include "net_ipset_node.h"

void net_ipset_node_print(write_stream_t ws, net_ipset_node_t node) {
    stream_printf(
        ws,
        "nonterminal(x%u? " NET_IPSET_NODE_ID_FORMAT ": " NET_IPSET_NODE_ID_FORMAT ")",
        node->m_variable,
        NET_IPSET_NODE_ID_VALUES(node->m_high),
        NET_IPSET_NODE_ID_VALUES(node->m_low));
}

uint32_t net_ipset_node_hash(void * user_data, const void * data) {
    net_ipset_node_t node = (net_ipset_node_t)data;
    uint32_t hash = 0xf3b7dc44;
    hash = cpe_hash_variable(hash, node->m_variable);
    hash = cpe_hash_variable(hash, node->m_low);
    hash = cpe_hash_variable(hash, node->m_high);
    return hash;
}

uint8_t net_ipset_node_eq(void * user_data, const void * l, const void * r) {
    net_ipset_node_t node1 = (net_ipset_node_t)l;
    net_ipset_node_t node2 = (net_ipset_node_t)r;

    if (node1 == node2) return 1;

    return
        (node1->m_variable == node2->m_variable) &&
        (node1->m_low == node2->m_low) &&
        (node1->m_high == node2->m_high);
}

