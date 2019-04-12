#include "assert.h"
#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_entry_alias_i.h"
#include "net_dns_task_i.h"

net_dns_entry_t
net_dns_entry_create(net_dns_manage_t manage, const char * hostname) {
    size_t hostname_len = strlen(hostname) + 1;

    uint8_t use_cache = hostname_len <= CPE_ENTRY_SIZE(net_dns_entry, m_hostname_buf);

    net_dns_entry_t entry;

    if (use_cache) {
        entry = TAILQ_FIRST(&manage->m_free_entries);
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
    }
    else {
        entry = mem_alloc(
            manage->m_alloc,
            sizeof(struct net_dns_entry) + (hostname_len - CPE_TYPE_ARRAY_SIZE(struct net_dns_entry, m_hostname_buf)));
        if (entry == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: entry alloc fail!");
            return NULL;
        }
    }

    entry->m_manage = manage;
    entry->m_hostname = entry->m_hostname_buf;
    entry->m_expire_time_s = 0;
    memcpy(entry->m_hostname_buf, hostname, hostname_len);
    entry->m_task = NULL;
    TAILQ_INIT(&entry->m_origins);
    TAILQ_INIT(&entry->m_cnames);
    TAILQ_INIT(&entry->m_items);
    
    cpe_hash_entry_init(&entry->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_entries, entry) != 0) {
        CPE_ERROR(manage->m_em, "dns-cli: entry duplicate!");
        if (use_cache) {
            TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
        }
        else {
            mem_free(manage->m_alloc, entry);
        }
        return NULL;
    }
    
    return entry;
}

void net_dns_entry_free(net_dns_entry_t entry) {
    net_dns_manage_t manage = entry->m_manage;
    size_t hostname_len = strlen(entry->m_hostname) + 1;
    uint8_t use_cache = hostname_len <= CPE_ENTRY_SIZE(net_dns_entry, m_hostname_buf);

    if (entry->m_task) {
        net_dns_task_free(entry->m_task);
        assert(entry->m_task == NULL);
    }

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

    if (use_cache) {
        TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
    }
    else {
        mem_free(manage->m_alloc, entry);
    }
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

net_dns_task_t net_dns_entry_task(net_dns_entry_t entry) {
    return entry->m_task;
}

const char * net_dns_entry_hostname(net_dns_entry_t entry) {
    return entry->m_hostname;
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
net_dns_entry_find(net_dns_manage_t manage, const char * hostname) {
    struct net_dns_entry key;
    key.m_hostname = hostname;
    return cpe_hash_table_find(&manage->m_entries, &key);
}

net_dns_entry_item_t
net_dns_entry_select_item(net_dns_entry_t entry, net_dns_item_select_policy_t policy) {
    net_dns_entry_item_t item = NULL;
    net_dns_entry_item_t check, next;

    struct net_dns_entry_item_it item_it;
    net_dns_entry_items(entry, &item_it, 1);

    for(check = net_dns_entry_item_it_next(&item_it); check; check = next) {
        next = net_dns_entry_item_it_next(&item_it);
        assert(next != check);

        if (net_address_type(check->m_address) == net_address_domain) continue;

        /* if (item) { */
        /*     switch(entry->m_manage->m_mode) { */
        /*     case net_dns_ipv4_first: */
        /*         if (net_address_type(check->m_address) == net_address_ipv6) { */
        /*             if (net_address_type(item->m_address) == net_address_ipv4) continue; */
        /*         } */
        /*         else { */
        /*             assert(net_address_type(check->m_address) == net_address_ipv4); */
        /*             if (net_address_type(item->m_address) == net_address_ipv6) { */
        /*                 item = NULL; */
        /*             } */
        /*         } */
        /*         break; */
        /*     case net_dns_ipv6_first: */
        /*         if (net_address_type(check->m_address) == net_address_ipv6) { */
        /*             if (net_address_type(item->m_address) == net_address_ipv4) { */
        /*                 item = NULL; */
        /*             } */
        /*         } */
        /*         else { */
        /*             assert(net_address_type(check->m_address) == net_address_ipv4); */
        /*             if (net_address_type(item->m_address) == net_address_ipv6) { */
        /*                 continue; */
        /*             } */
        /*         } */
        /*         break; */
        /*     } */
        /* } */
        
        switch(policy) {
        case net_dns_item_select_policy_first:
            if (item == NULL) {
                item = check;
            }
            break;
        }
    }

    return item;
}

struct net_dns_entry_address_it_data {
    net_dns_entry_t m_root;
    net_dns_entry_alias_t m_entry_alias;
    net_dns_entry_t m_entry;
    net_dns_entry_item_t m_item;
};

static void net_dns_entry_address_it_data_go_next(struct net_dns_entry_address_it_data * data) {
    while(data->m_entry) {
        if (data->m_item) {
            data->m_item = TAILQ_NEXT(data->m_item, m_next_for_entry);
        }
        else {
            data->m_item = TAILQ_FIRST(&data->m_entry->m_items);
        }

        if (data->m_item) break;
        
        assert(data->m_item == NULL);

        if (data->m_entry == data->m_root) {
            data->m_entry = NULL;
            break;
        }

        if (data->m_entry_alias == NULL) {
            data->m_entry = NULL;
            break;
        }
        
        net_dns_entry_alias_t next_alias = TAILQ_NEXT(data->m_entry_alias, m_next_for_origin);
        if (next_alias) {
            data->m_entry_alias = next_alias;
            data->m_entry = next_alias->m_as;
            while(!TAILQ_EMPTY(&data->m_entry->m_cnames)) {
                data->m_entry_alias = TAILQ_FIRST(&data->m_entry->m_cnames);
                data->m_entry = data->m_entry_alias->m_as;
            }
        }
        else {
            data->m_entry = data->m_entry_alias->m_origin;

            if (data->m_entry == data->m_root) {
                data->m_entry_alias = NULL;
            }
            else {
                TAILQ_FOREACH(data->m_entry_alias, &data->m_entry->m_origins, m_next_for_as) {
                    if (data->m_entry_alias->m_as == data->m_entry) break;
                }
            }
        }
    }
}

static net_address_t net_dns_entry_address_it_recursive_next(net_address_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_address_t r = data->m_item->m_address;
    net_dns_entry_address_it_data_go_next(data);

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
    if (recursive) {
        struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

        assert(sizeof(struct net_dns_entry_address_it_data) <= sizeof(it->data));
        
        data->m_root = entry;
        data->m_item = NULL;

        data->m_entry = data->m_root;
        data->m_entry_alias = NULL;
        while(!TAILQ_EMPTY(&data->m_entry->m_cnames)) {
            data->m_entry_alias = TAILQ_FIRST(&data->m_entry->m_cnames);
            data->m_entry = data->m_entry_alias->m_as;
        }
        
        net_dns_entry_address_it_data_go_next(data);
        it->next = net_dns_entry_address_it_recursive_next;
    }
    else {
        net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;
        *item = TAILQ_FIRST(&entry->m_items);
        it->next = net_dns_entry_address_it_basic_next;
    }
}

static net_dns_entry_item_t net_dns_entry_item_it_recursive_next(net_dns_entry_item_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_dns_entry_item_t r = data->m_item;
    net_dns_entry_address_it_data_go_next(data);

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
    if (recursive) {
        struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

        assert(sizeof(struct net_dns_entry_address_it_data) <= sizeof(it->data));
        
        data->m_root = entry;
        data->m_item = NULL;

        data->m_entry = data->m_root;
        data->m_entry_alias = NULL;
        while(!TAILQ_EMPTY(&data->m_entry->m_cnames)) {
            data->m_entry_alias = TAILQ_FIRST(&data->m_entry->m_cnames);
            data->m_entry = data->m_entry_alias->m_as;
        }

        net_dns_entry_address_it_data_go_next(data);
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
    return cpe_hash_str(o->m_hostname, strlen(o->m_hostname));
}

int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r, void * user_data) {
    return strcmp(l->m_hostname, r->m_hostname) == 0 ? 1 : 0;
}
