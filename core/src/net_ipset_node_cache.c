#include "cpe/pal/pal_types.h"
#include "cpe/utils/hash_standalone.h"
#include "net_ipset_node_cache.h"

#define NET_IPSET_NULL_INDEX ((net_ipset_variable) -1)

static net_ipset_value net_ipset_node_cache_alloc_node(net_ipset_node_cache_t cache);

net_ipset_node_cache_t net_ipset_node_cache_create(mem_allocrator_t alloc, error_monitor_t em) {
    net_ipset_node_cache_t cache = mem_alloc(alloc, sizeof(struct net_ipset_node_cache));
    if (cache == NULL) {
        CPE_ERROR(em, "ipset_node_cache_create: alloc fail");
        return NULL;
    }

    cache->m_alloc = alloc;
    cpe_array_init(&cache->m_chunks, alloc);
    cache->m_largest_index = 0;
    cache->m_free_list = NET_IPSET_NULL_INDEX;

    cache->m_node_cache = cpe_hash_table_standalone_create(alloc, 0);
    if (cache->m_node_cache == NULL) {
        CPE_ERROR(em, "ipset_node_cache_create: create hash table fail");
        mem_free(alloc, cache);
        return NULL;
    }
    
    cpe_hash_table_standalone_set_hash(cache->m_node_cache, net_ipset_node_hash);
    cpe_hash_table_standalone_set_equals(cache->m_node_cache, net_ipset_node_eq);
    
    return cache;
}

void net_ipset_node_cache_free(net_ipset_node_cache_t cache) {
    size_t  i;
    for (i = 0; i < cpe_array_size(&cache->m_chunks); i++) {
        mem_free(cache->m_alloc, cpe_array_at(&cache->m_chunks, i));
    }
    cpe_array_fini(&cache->m_chunks);

    cpe_hash_table_standalone_free(cache->m_node_cache);
    
    mem_free(cache->m_alloc, cache);
}

net_ipset_node_id
net_ipset_node_cache_nonterminal(
    net_ipset_node_cache_t cache, net_ipset_variable variable, net_ipset_node_id low, net_ipset_node_id high)
{
    if (low == high) {
        net_ipset_node_decref(cache, high);
        return low;
    }

    struct net_ipset_node  search_node;
    search_node.m_variable = variable;
    search_node.m_low = low;
    search_node.m_high = high;

    uint8_t is_new;
    cpe_hash_table_standalone_entry_t entry =
        cpe_hash_table_standalone_get_or_create(cache->m_node_cache, &search_node, &is_new);
    if (!is_new) {
        /* There's already a node with these contents, so return its ID. */
        net_ipset_node_id  node_id = (net_ipset_node_id)(ptr_int_t)entry->value;
        net_ipset_node_incref(cache, node_id);
        net_ipset_node_decref(cache, low);
        net_ipset_node_decref(cache, high);
        return node_id;
    }
    else {
        net_ipset_value new_index = net_ipset_node_cache_alloc_node(cache);
        net_ipset_node_id new_node_id = net_ipset_nonterminal_node_id(new_index);
        net_ipset_node_t real_node = net_ipset_node_cache_get_nonterminal_by_index(cache, new_index);
        real_node->m_refcount = 1;
        real_node->m_variable = variable;
        real_node->m_low = low;
        real_node->m_high = high;

        entry->key = real_node;
        entry->value = (void *)(ptr_int_t)new_node_id;
        return new_node_id;
    }
}

static net_ipset_value
net_ipset_node_cache_alloc_node(net_ipset_node_cache_t cache) {
    if (cache->m_free_list == NET_IPSET_NULL_INDEX) {
        net_ipset_value  next_index = cache->m_largest_index++;
        net_ipset_value  chunk_index = next_index >> NET_IPSET_BDD_NODE_CACHE_BIT_SIZE;
        if (chunk_index >= cpe_array_size(&cache->m_chunks)) {
            net_ipset_node_t new_chunk =
                mem_alloc(cache->m_alloc, NET_IPSET_BDD_NODE_CACHE_SIZE * sizeof(struct net_ipset_node));
            cpe_array_append(&cache->m_chunks, new_chunk);
        }
        return next_index;
    }
    else {
        net_ipset_value  next_index = cache->m_free_list;
        net_ipset_node_t node = net_ipset_node_cache_get_nonterminal_by_index(cache, next_index);
        cache->m_free_list = node->m_refcount;
        return next_index;
    }
}

struct net_ipset_fake_node {
    net_ipset_variable current_var;
    net_ipset_variable var_count;
    net_ipset_assignment_func_t assignment;
    const void * user_data;
    net_ipset_value value;
};
static struct net_ipset_fake_node fake_terminal_0 = { 0, 0, NULL, 0, 0 };

static net_ipset_node_id
net_ipset_apply_ite(
    net_ipset_node_cache_t cache, struct net_ipset_fake_node *f, net_ipset_value g, net_ipset_node_id h)
{
    net_ipset_node_id  h_low;
    net_ipset_node_id  h_high;
    net_ipset_node_id  result_low;
    net_ipset_node_id  result_high;

    if (f->current_var == f->var_count) {
        return f->value == 0
            ? net_ipset_node_incref(cache, h)
            : net_ipset_terminal_node_id(g);
    }

    if (h == net_ipset_terminal_node_id(g)) {
        return h;
    }

    if (net_ipset_node_get_type(h) == NET_IPSET_NONTERMINAL_NODE) {
        net_ipset_node_t h_node = net_ipset_node_cache_get_nonterminal(cache, h);

        if (h_node->m_variable < f->current_var) {
            result_high = net_ipset_apply_ite(cache, f, g, h_node->m_high);
            result_low = net_ipset_apply_ite(cache, f, g, h_node->m_low);
            return net_ipset_node_cache_nonterminal(cache, h_node->m_variable, result_low, result_high);
        }
        else if (h_node->m_variable == f->current_var) {
            h_low = h_node->m_low;
            h_high = h_node->m_high;
        }
        else {
            h_low = h;
            h_high = h;
        }
    }
    else {
        h_low = h;
        h_high = h;
    }

    if (f->assignment(f->user_data, f->current_var)) {
        f->current_var++;
        result_high = net_ipset_apply_ite(cache, f, g, h_high);
        f->current_var--;
        fake_terminal_0.current_var = f->var_count;
        fake_terminal_0.var_count = f->var_count;
        result_low = net_ipset_apply_ite(cache, &fake_terminal_0, g, h_low);
    }
    else {
        fake_terminal_0.current_var = f->var_count;
        fake_terminal_0.var_count = f->var_count;
        result_high = net_ipset_apply_ite(cache, &fake_terminal_0, g, h_high);
        f->current_var++;
        result_low = net_ipset_apply_ite(cache, f, g, h_low);
        f->current_var--;
    }

    return net_ipset_node_cache_nonterminal(cache, f->current_var, result_low, result_high);
}

net_ipset_node_id
net_ipset_node_insert(
    net_ipset_node_cache_t cache, net_ipset_node_id node,
    net_ipset_assignment_func_t assignment, const void *user_data,
    net_ipset_variable var_count, net_ipset_value value)
{
    struct net_ipset_fake_node f = { 0, var_count, assignment, user_data, 1 };
    return net_ipset_apply_ite(cache, &f, value, node);
}

net_ipset_value
net_ipset_node_evaluate(
    net_ipset_node_cache_t cache, net_ipset_node_id node_id,
    net_ipset_assignment_func_t assignment, const void *user_data)
{
    net_ipset_node_id  curr_node_id = node_id;

    while (net_ipset_node_get_type(curr_node_id) == NET_IPSET_NONTERMINAL_NODE) {
        net_ipset_node_t node = net_ipset_node_cache_get_nonterminal(cache, curr_node_id);
        uint8_t this_value = assignment(user_data, node->m_variable);

        if (this_value) {
            curr_node_id = node->m_high;
        }
        else {
            curr_node_id = node->m_low;
        }
    }

    return net_ipset_terminal_value(curr_node_id);
}

net_ipset_node_id
net_ipset_node_incref(net_ipset_node_cache_t cache, net_ipset_node_id node_id) {
    if (net_ipset_node_get_type(node_id) == NET_IPSET_NONTERMINAL_NODE) {
        net_ipset_node_t node = net_ipset_node_cache_get_nonterminal(cache, node_id);
        node->m_refcount++;
    }
    return node_id;
}

void net_ipset_node_decref(net_ipset_node_cache_t cache, net_ipset_node_id node_id) {
    if (net_ipset_node_get_type(node_id) == NET_IPSET_NONTERMINAL_NODE) {
        net_ipset_node_t node = net_ipset_node_cache_get_nonterminal(cache, node_id);
        if (--node->m_refcount == 0) {
            net_ipset_node_decref(cache, node->m_low);
            net_ipset_node_decref(cache, node->m_high);
            cpe_hash_table_standalone_delete(cache->m_node_cache, node, NULL, NULL);

            node->m_refcount = cache->m_free_list;
            cache->m_free_list = net_ipset_nonterminal_value(node_id);
        }
    }
}

uint8_t net_ipset_node_cache_nodes_equal(
    net_ipset_node_cache_t cache1, net_ipset_node_id node_id1,
    net_ipset_node_cache_t cache2, net_ipset_node_id node_id2)
{
    net_ipset_node_t node1;
    net_ipset_node_t node2;

    if (net_ipset_node_get_type(node_id1) != net_ipset_node_get_type(node_id2)) {
        return 0;
    }

    if (net_ipset_node_get_type(node_id1) == NET_IPSET_TERMINAL_NODE) {
        return node_id1 == node_id2 ? 1 : 0;
    }

    node1 = net_ipset_node_cache_get_nonterminal(cache1, node_id1);
    node2 = net_ipset_node_cache_get_nonterminal(cache2, node_id2);
    return
        (node1->m_variable == node2->m_variable) &&
        net_ipset_node_cache_nodes_equal(cache1, node1->m_low, cache2, node2->m_low) &&
        net_ipset_node_cache_nodes_equal(cache1, node1->m_high, cache2, node2->m_high);
}

size_t net_ipset_node_reachable_count(net_ipset_node_cache_t cache, net_ipset_node_id node) {
    cpe_hash_table_standalone_t visited = cpe_pointer_hash_table_create(cache->m_alloc, 0);

    cpe_array(net_ipset_node_id) queue;
    cpe_array_init(&queue, NULL);

    if (net_ipset_node_get_type(node) == NET_IPSET_NONTERMINAL_NODE) {
        cpe_array_append(&queue, node);
    }

    size_t  node_count = 0;
    while (!cpe_array_is_empty(&queue)) {
        net_ipset_node_id curr = cpe_array_at(&queue, --queue.size);

        if (cpe_hash_table_standalone_get(visited, (void *) (ptr_int_t) curr) == NULL) {
            cpe_hash_table_standalone_put(
                visited, (void *) (ptr_int_t) curr, (void *) (ptr_int_t) 1, NULL, NULL, NULL);

            node_count++;

            net_ipset_node_t node = net_ipset_node_cache_get_nonterminal(cache, curr);
            if (net_ipset_node_get_type(node->m_low) == NET_IPSET_NONTERMINAL_NODE) {
                cpe_array_append(&queue, node->m_low);
            }

            if (net_ipset_node_get_type(node->m_high) == NET_IPSET_NONTERMINAL_NODE) {
                cpe_array_append(&queue, node->m_high);
            }
        }
    }

    cpe_hash_table_standalone_free(visited);
    cpe_array_fini(&queue);
    return node_count;
}

size_t net_ipset_node_memory_size(net_ipset_node_cache_t cache, net_ipset_node_id node) {
    return net_ipset_node_reachable_count(cache, node) * sizeof(struct net_ipset_node);
}
