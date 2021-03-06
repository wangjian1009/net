#include "assert.h"
#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_entry_alias_i.h"

net_dns_entry_t
net_dns_entry_create(net_dns_manage_t manage, net_address_t hostname) {
    assert(net_address_type(hostname) == net_address_domain);

    net_dns_entry_t entry = TAILQ_FIRST(&manage->m_free_entries);
    if (entry) {
        TAILQ_REMOVE(&manage->m_free_entries, entry, m_next);
    }
    else {
        entry = mem_alloc(manage->m_alloc, sizeof(struct net_dns_entry));
        if (entry == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: entry alloc fail!");
            return NULL;
        }
    }

    entry->m_manage = manage;
    entry->m_hostname = net_address_create_domain(manage->m_schedule, (const char *)net_address_data(hostname), 0, NULL);
    if (entry->m_hostname == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: create hostname fail!");
        TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
        return NULL;
    }
    
    entry->m_expire_time_s = 0;
    TAILQ_INIT(&entry->m_origins);
    TAILQ_INIT(&entry->m_cnames);
    TAILQ_INIT(&entry->m_items);
    
    cpe_hash_entry_init(&entry->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_entries, entry) != 0) {
        CPE_ERROR(manage->m_em, "dns-cli: entry duplicate!");
        net_address_free(entry->m_hostname);
        TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
        return NULL;
    }
    
    return entry;
}

void net_dns_entry_free(net_dns_entry_t entry) {
    net_dns_manage_t manage = entry->m_manage;

    while(!TAILQ_EMPTY(&entry->m_origins)) {
        net_dns_entry_alias_free(TAILQ_FIRST(&entry->m_origins));
    }
    
    while(!TAILQ_EMPTY(&entry->m_cnames)) {
        net_dns_entry_alias_free(TAILQ_FIRST(&entry->m_cnames));
    }
    
    while(!TAILQ_EMPTY(&entry->m_items)) {
        net_dns_entry_item_free(TAILQ_FIRST(&entry->m_items));
    }

    cpe_hash_table_remove_by_ins(&manage->m_entries, entry);

    net_address_free(entry->m_hostname);
    entry->m_hostname = NULL;
    
    TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
}

void net_dns_entry_real_free(net_dns_entry_t entry) {
    net_dns_manage_t manage = entry->m_manage;
    TAILQ_REMOVE(&manage->m_free_entries, entry, m_next);
    mem_free(manage->m_alloc, entry);
}

void net_dns_entry_free_all(net_dns_manage_t manage) {
    struct cpe_hash_it entry_it;
    net_dns_entry_t entry;

    cpe_hash_it_init(&entry_it, &manage->m_entries);

    entry = cpe_hash_it_next(&entry_it);
    while(entry) {
        net_dns_entry_t next = cpe_hash_it_next(&entry_it);
        net_dns_entry_free(entry);
        entry = next;
    }
}

net_address_t net_dns_entry_hostname(net_dns_entry_t entry) {
    return entry->m_hostname;
}

const char * net_dns_entry_hostname_str(net_dns_entry_t entry) {
    return (const char *)net_address_data(entry->m_hostname);
}

uint8_t net_dns_entry_is_origin_of(net_dns_entry_t entry, net_dns_entry_t as) {
    net_dns_entry_alias_t check;

    TAILQ_FOREACH(check, &entry->m_cnames, m_next_for_origin) {
        if (check->m_as == as) return 1;
    }

    TAILQ_FOREACH(check, &entry->m_cnames, m_next_for_origin) {
        if (net_dns_entry_is_origin_of(check->m_as, as)) return 1;
    }
    
    return 0;
}

net_dns_entry_t
net_dns_entry_find(net_dns_manage_t manage, net_address_t hostname) {
    struct net_dns_entry key;
    key.m_hostname = hostname;
    return cpe_hash_table_find(&manage->m_entries, &key);
}

net_dns_entry_item_t
net_dns_entry_select_item(net_dns_entry_t entry, net_dns_item_select_policy_t policy, net_dns_query_type_t query_type) {
    net_dns_manage_t manage = entry->m_manage;
    net_dns_entry_item_t item = NULL;
    net_dns_entry_item_t check, next;

    assert(entry);

    struct net_dns_entry_item_it item_it;
    net_dns_entry_items(entry, &item_it, 1);

    uint32_t protect = 0;
    uint32_t protect_limit = 500;
    for(check = net_dns_entry_item_it_next(&item_it); check && protect < protect_limit; check = next, protect++) {
        next = net_dns_entry_item_it_next(&item_it);
        assert(next != check);

        switch(net_address_type(check->m_address)) {
        case net_address_ipv4:
            if (query_type != net_dns_query_ipv4 && query_type != net_dns_query_ipv4v6) continue;
            break;
        case net_address_ipv6:
            if (query_type != net_dns_query_ipv6 && query_type != net_dns_query_ipv4v6) continue;
            break;
        case net_address_domain:
            if (query_type != net_dns_query_domain) continue;
            break;
        case net_address_local:
            continue;
            break;
        }

        switch(policy) {
        case net_dns_item_select_policy_first:
            if (item == NULL) {
                item = check;
            }
            break;
        }
    }

    if (protect >= protect_limit) {
        struct cpe_hash_it dump_entry_it;
        net_dns_entry_t dump_entry;

        CPE_INFO(
            manage->m_em, "net: dns: select entry of %s: item limit protected, dump cache!",
            (const char *)net_address_data(entry->m_hostname));
        
        cpe_hash_it_init(&dump_entry_it, &manage->m_entries);
        while((dump_entry = cpe_hash_it_next(&dump_entry_it))) {
            CPE_INFO(
                manage->m_em, "            entry %s(%p)",
                (const char *)net_address_data(dump_entry->m_hostname), dump_entry);

            net_dns_entry_alias_t alias;
            TAILQ_FOREACH(alias, &dump_entry->m_cnames, m_next_for_origin) {
                CPE_INFO(
                    manage->m_em, "                cname %s(%p)",
                    (const char *)net_address_data(alias->m_as->m_hostname), alias->m_as);
            }

            net_dns_entry_item_t item;
            TAILQ_FOREACH(item, &dump_entry->m_items, m_next_for_entry) {
                CPE_INFO(
                    manage->m_em, "                %s",
                    net_address_host(net_dns_manage_tmp_buffer(manage), item->m_address));
            }
        }
        
        assert(protect < protect_limit);
    }

    return item;
}

struct net_dns_entry_recursive_it_data {
    uint32_t m_visit_id;
    uint16_t m_stack_size;
    net_dns_entry_item_t m_item;
};

static 
struct net_dns_visit_node * 
net_dns_entry_recursive_it_data_append_node(
    net_dns_manage_t manage, struct net_dns_entry_recursive_it_data * data, net_dns_entry_t entry)
{
    if (data->m_visit_id != manage->m_visit_id_current) {
        CPE_ERROR(manage->m_em, "net: dns: recursive it visit id mismatch");
        return NULL;
    }
    
    if (data->m_stack_size >= CPE_ARRAY_SIZE(manage->m_visit_stack)) {
        CPE_ERROR(manage->m_em, "net: dns: recursive it overflow");
        return NULL;
    }
    
    struct net_dns_visit_node * node = &manage->m_visit_stack[data->m_stack_size];
    
    node->m_entry = entry;
    node->m_child = TAILQ_FIRST(&entry->m_cnames);
    data->m_item = TAILQ_FIRST(&entry->m_items);
    data->m_stack_size++;
    
    return node;
}

static 
struct net_dns_visit_node * 
net_dns_entry_recursive_it_data_current_node(net_dns_manage_t manage, struct net_dns_entry_recursive_it_data * data) {
    if (data->m_visit_id != manage->m_visit_id_current) {
        CPE_ERROR(manage->m_em, "net: dns: recursive it visit id mismatch");
        return NULL;
    }
    
    if (data->m_stack_size >= CPE_ARRAY_SIZE(manage->m_visit_stack)) {
        CPE_ERROR(manage->m_em, "net: dns: recursive it overflow");
        return NULL;
    }

    return data->m_stack_size == 0 ? NULL : &manage->m_visit_stack[data->m_stack_size - 1];
}

static void net_dns_entry_recursive_it_data_go_next(net_dns_manage_t manage, struct net_dns_entry_recursive_it_data * data) {
    assert(data->m_stack_size <= CPE_ARRAY_SIZE(manage->m_visit_stack));

    struct net_dns_visit_node * node = net_dns_entry_recursive_it_data_current_node(manage, data);
PROCESS_NODE:    
    if (node) {
        /*优先遍历当前节点 */
        if (data->m_item) {
            data->m_item = TAILQ_NEXT(data->m_item, m_next_for_entry);
            if (data->m_item != NULL) return;
        }

        /*然后尝试遍历子节点 */
        assert(data->m_item == NULL);
        while(node->m_child) {
            if (node->m_child->m_visit_id < data->m_visit_id) {
                /*没有访问过的子节点 */
                net_dns_entry_t next_entry = node->m_child->m_as;
                node->m_child->m_visit_id = data->m_visit_id;
                node->m_child = TAILQ_NEXT(node->m_child, m_next_for_origin);
                node = net_dns_entry_recursive_it_data_append_node(manage, data, next_entry);
                if (data->m_item != NULL) return;
                goto PROCESS_NODE;
            }
            else {
                node->m_child = TAILQ_NEXT(node->m_child, m_next_for_origin);
            }
        }
        
        /*最后回溯到父节点 */
        assert(data->m_stack_size > 0);
        data->m_stack_size--;
        node = net_dns_entry_recursive_it_data_current_node(manage, data);
        goto PROCESS_NODE;
    }
    
    if (node == NULL) {
        data->m_item = NULL;
    }
}

static net_address_t net_dns_entry_address_it_recursive_next(net_address_it_t it) {
    struct net_dns_entry_recursive_it_data * data = (struct net_dns_entry_recursive_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_address_t r = data->m_item->m_address;
    net_dns_manage_t manage = data->m_item->m_entry->m_manage;
    net_dns_entry_recursive_it_data_go_next(manage, data);

    return r;
}

static net_address_t net_dns_entry_address_it_basic_next(net_address_it_t it) {
    net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;

    net_dns_entry_item_t r = *item;
    if (r == NULL) return NULL;
    
    *item = TAILQ_NEXT(r, m_next_for_entry);
    
    return r->m_address;
}

void net_dns_entry_addresses(net_dns_entry_t entry, net_address_it_t it, uint8_t recursive) {
    net_dns_manage_t manage = entry->m_manage;
    
    if (recursive) {
        struct net_dns_entry_recursive_it_data * data = (struct net_dns_entry_recursive_it_data *)it->data;

        assert(sizeof(struct net_dns_entry_recursive_it_data) <= sizeof(it->data));

        manage->m_visit_id_current = ++manage->m_visit_id_max;
        data->m_visit_id = manage->m_visit_id_current;
        data->m_stack_size = 0;
        net_dns_entry_recursive_it_data_append_node(manage, data, entry);
        if (data->m_item == NULL) {
            net_dns_entry_recursive_it_data_go_next(manage, data);
        }

        it->next = net_dns_entry_address_it_recursive_next;
    }
    else {
        net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;
        *item = TAILQ_FIRST(&entry->m_items);
        it->next = net_dns_entry_address_it_basic_next;
    }
}

static net_dns_entry_item_t net_dns_entry_item_it_recursive_next(net_dns_entry_item_it_t it) {
    struct net_dns_entry_recursive_it_data * data = (struct net_dns_entry_recursive_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_dns_entry_item_t r = data->m_item;
    net_dns_manage_t manage = data->m_item->m_entry->m_manage;
    net_dns_entry_recursive_it_data_go_next(manage, data);

    return r;
}

static net_dns_entry_item_t net_dns_entry_item_it_basic_next(net_dns_entry_item_it_t it) {
    net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;

    net_dns_entry_item_t r = *item;

    if (r) {
        *item = TAILQ_NEXT(r, m_next_for_entry);
    }

    return r;
}

void net_dns_entry_items(net_dns_entry_t entry, net_dns_entry_item_it_t it, uint8_t recursive) {
    net_dns_manage_t manage = entry->m_manage;
    
    assert(entry);

    if (recursive) {
        struct net_dns_entry_recursive_it_data * data = (struct net_dns_entry_recursive_it_data *)it->data;

        assert(sizeof(struct net_dns_entry_recursive_it_data) <= sizeof(it->data));

        manage->m_visit_id_current = ++manage->m_visit_id_max;
        data->m_visit_id = manage->m_visit_id_current;
        data->m_stack_size = 0;
        net_dns_entry_recursive_it_data_append_node(manage, data, entry);
        if (data->m_item == NULL) {
            net_dns_entry_recursive_it_data_go_next(manage, data);
        }
        it->next = net_dns_entry_item_it_recursive_next;
    }
    else {
        net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;
        *item = TAILQ_FIRST(&entry->m_items);
        it->next = net_dns_entry_item_it_basic_next;
    }
}

static net_dns_entry_t net_dns_entry_cname_it_next(net_dns_entry_it_t it) {
    net_dns_entry_alias_t * data = (net_dns_entry_alias_t *)it->data;

    if (*data == NULL) return NULL;

    net_dns_entry_alias_t r = *data;
    *data = TAILQ_NEXT(r, m_next_for_origin);

    return r->m_as;
}

void net_dns_entrys(net_dns_entry_t entry, net_dns_entry_it_t it) {
    net_dns_entry_alias_t * data = (net_dns_entry_alias_t *)it->data;
    *data = TAILQ_FIRST(&entry->m_cnames);
    it->next = net_dns_entry_cname_it_next;
}

void net_dns_entry_clear(net_dns_entry_t entry) {
    while(!TAILQ_EMPTY(&entry->m_items)) {
        net_dns_entry_item_free(TAILQ_FIRST(&entry->m_items));
    }

    while(!TAILQ_EMPTY(&entry->m_cnames)) {
        net_dns_entry_alias_t alias = TAILQ_FIRST(&entry->m_cnames);
        assert(alias->m_origin == entry);
        net_dns_entry_free(alias->m_as);
    }
}

uint32_t net_dns_entry_hash(net_dns_entry_t o, void * user_data) {
    return net_address_hash_without_port(o->m_hostname);
}

int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r, void * user_data) {
    return net_address_cmp_without_port(l->m_hostname, r->m_hostname) == 0 ? 1 : 0;
}
