#include "assert.h"
#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
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
    memcpy(entry->m_hostname_buf, hostname, hostname_len);
    entry->m_task = NULL;
    entry->m_main = NULL; 
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

    if (entry->m_main) {
        net_dns_entry_set_main(entry, NULL);
        assert(entry->m_main == NULL);
    }

    while(!TAILQ_EMPTY(&entry->m_cnames)) {
        net_dns_entry_free(TAILQ_FIRST(&entry->m_cnames));
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

net_dns_entry_t net_dns_entry_effective(net_dns_entry_t entry) {
    while(entry->m_main) {
        entry = entry->m_main;
    }

    return entry;
}

net_dns_entry_t net_dns_entry_main(net_dns_entry_t entry) {
    return entry->m_main;
}

int net_dns_entry_set_main(net_dns_entry_t entry, net_dns_entry_t main) {
    if (main && net_dns_entry_is_main_of(entry, main)) {
        CPE_ERROR(entry->m_manage->m_em, "dns-cli: entry %s set main %s: circle!", entry->m_hostname, main->m_hostname);
        return -1;
    }
        
    if (entry->m_main) {
        TAILQ_REMOVE(&entry->m_main->m_cnames, entry, m_next_for_main);
    }

    entry->m_main = main;

    if (entry->m_main) {
        TAILQ_INSERT_TAIL(&entry->m_main->m_cnames, entry, m_next_for_main);
    }
    
    return 0;
}

uint8_t net_dns_entry_is_main_of(net_dns_entry_t entry, net_dns_entry_t check_start) {
    while(check_start) {
        if (check_start == entry) return 1;
        check_start = check_start->m_main;
    }
    return 0;
}

const char * net_dns_entry_hostname(net_dns_entry_t entry) {
    return entry->m_hostname;
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
    net_dns_entry_item_t check;

    struct net_dns_entry_item_it item_it;
    net_dns_entry_items(entry, &item_it);
    
    switch(policy) {
    case net_dns_item_select_policy_first:
        item = net_dns_entry_item_it_next(&item_it);
        break;
    case net_dns_item_select_policy_max_ttl:
        while((check = net_dns_entry_item_it_next(&item_it))) {
            if (check->m_expire_time_ms == 0) {
                item = check;
                break;
            }

            if (item == NULL || check->m_expire_time_ms > item->m_expire_time_ms) {
                item = check;
            }
        }
        break;
    }

    return item;
}

struct net_dns_entry_address_it_data {
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
        if (!TAILQ_EMPTY(&data->m_entry->m_cnames)) {
            data->m_entry = TAILQ_FIRST(&data->m_entry->m_cnames);
        }
        else if (data->m_entry->m_main) {
            data->m_entry = TAILQ_NEXT(data->m_entry, m_next_for_main);
        }
        else {
            data->m_entry = NULL;
        }
    }
}

static net_address_t net_dns_entry_address_it_next(net_address_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_address_t r = data->m_item->m_address;
    net_dns_entry_address_it_data_go_next(data);

    return r;
}

void net_dns_entry_addresses(net_dns_entry_t entry, net_address_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;
    data->m_entry = entry;
    data->m_item = NULL;
    net_dns_entry_address_it_data_go_next(data);
    it->next = net_dns_entry_address_it_next;
}

static net_dns_entry_item_t net_dns_entry_item_it_do_next(net_dns_entry_item_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;

    if (data->m_item == NULL) return NULL;

    net_dns_entry_item_t r = data->m_item;
    net_dns_entry_address_it_data_go_next(data);

    return r;
}

void net_dns_entry_items(net_dns_entry_t entry, net_dns_entry_item_it_t it) {
    struct net_dns_entry_address_it_data * data = (struct net_dns_entry_address_it_data *)it->data;
    data->m_entry = entry;
    data->m_item = NULL;
    net_dns_entry_address_it_data_go_next(data);
    it->next = net_dns_entry_item_it_do_next;
}

uint32_t net_dns_entry_hash(net_dns_entry_t o, void * user_data) {
    return cpe_hash_str(o->m_hostname, strlen(o->m_hostname));
}

int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r, void * user_data) {
    return strcmp(l->m_hostname, r->m_hostname) == 0 ? 1 : 0;
}
