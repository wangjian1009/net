#ifndef NET_IPSET_NODE_CACHE_H_INCLEDED
#define NET_IPSET_NODE_CACHE_H_INCLEDED
#include "cpe/utils/hash.h"
#include "cpe/utils/array.h"
#include "net_ipset_node.h"

NET_BEGIN_DECL

struct net_ipset_node_cache {
    mem_allocrator_t m_alloc;
    cpe_array(net_ipset_node_t) m_chunks;
    net_ipset_value m_largest_index;
    net_ipset_value m_free_list;
    cpe_hash_table_standalone_t m_node_cache;
};

#define NET_IPSET_BDD_NODE_CACHE_BIT_SIZE  6
#define NET_IPSET_BDD_NODE_CACHE_SIZE  (1 << NET_IPSET_BDD_NODE_CACHE_BIT_SIZE)
#define NET_IPSET_BDD_NODE_CACHE_MASK  (NET_IPSET_BDD_NODE_CACHE_SIZE - 1)

#define net_ipset_nonterminal_chunk_index(index) ((index) >> NET_IPSET_BDD_NODE_CACHE_BIT_SIZE)
#define net_ipset_nonterminal_chunk_offset(index) ((index) & NET_IPSET_BDD_NODE_CACHE_MASK)

#define net_ipset_node_cache_get_nonterminal_by_index(cache, index) \
    (&cpe_array_at(&(cache)->m_chunks, net_ipset_nonterminal_chunk_index((index))) \
     [net_ipset_nonterminal_chunk_offset((index))])

#define net_ipset_node_cache_get_nonterminal(cache, node_id) \
    (net_ipset_node_cache_get_nonterminal_by_index((cache), net_ipset_nonterminal_value((node_id))))

net_ipset_node_cache_t net_ipset_node_cache_create(mem_allocrator_t alloc, error_monitor_t em);
void net_ipset_node_cache_free(net_ipset_node_cache_t cache);

net_ipset_node_id
net_ipset_node_cache_nonterminal(
    net_ipset_node_cache_t cache,
    net_ipset_variable variable,
    net_ipset_node_id low, net_ipset_node_id high);

net_ipset_node_id
net_ipset_node_incref(net_ipset_node_cache_t cache, net_ipset_node_id node);

void net_ipset_node_decref(net_ipset_node_cache_t cache, net_ipset_node_id node);

size_t net_ipset_node_reachable_count(net_ipset_node_cache_t cache, net_ipset_node_id node);
size_t net_ipset_node_memory_size(net_ipset_node_cache_t cache, net_ipset_node_id node);

net_ipset_value
net_ipset_node_evaluate(
    net_ipset_node_cache_t cache, net_ipset_node_id node,
    net_ipset_assignment_func_t assignment, const void *user_data);

net_ipset_node_id
net_ipset_node_insert(
    net_ipset_node_cache_t cache, net_ipset_node_id node,
    net_ipset_assignment_func_t assignment, const void *user_data,
    net_ipset_variable variable_count, net_ipset_value value);

uint8_t net_ipset_node_cache_nodes_equal(
    net_ipset_node_cache_t cache1, net_ipset_node_id node_id1,
    net_ipset_node_cache_t cache2, net_ipset_node_id node_id2);

NET_END_DECL

#endif
