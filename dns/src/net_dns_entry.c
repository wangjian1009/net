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
                CPE_ERROR(manage->m_em, "dns: entry alloc fail!");
                return NULL;
            }
        }
    }
    else {
        entry = mem_alloc(
            manage->m_alloc,
            sizeof(struct net_dns_entry) + (hostname_len - CPE_TYPE_ARRAY_SIZE(struct net_dns_entry, m_hostname_buf)));
        if (entry == NULL) {
            CPE_ERROR(manage->m_em, "dns: entry alloc fail!");
            return NULL;
        }
    }

    entry->m_manage = manage;
    entry->m_hostname = entry->m_hostname_buf;
    memcpy(entry->m_hostname_buf, hostname, hostname_len);
    entry->m_task = NULL;
    TAILQ_INIT(&entry->m_items);
    
    cpe_hash_entry_init(&entry->m_hh);
    if (cpe_hash_table_insert_unique(&manage->m_entries, entry) != 0) {
        CPE_ERROR(manage->m_em, "dns: entry duplicate!");
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

    switch(policy) {
    case net_dns_item_select_policy_first:
        item = TAILQ_FIRST(&entry->m_items);
        break;
    case net_dns_item_select_policy_max_ttl:
        TAILQ_FOREACH(check, &entry->m_items, m_next_for_entry) {
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

static net_address_t net_dns_entry_address_it_next(net_address_it_t it) {
    net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;

    if (*item == NULL) return NULL;

    net_address_t r = (*item)->m_address;
    (*item) = TAILQ_NEXT(*item, m_next_for_entry);

    return r;
}

void net_dns_entry_addresses(net_dns_entry_t entry, net_address_it_t it) {
    net_dns_entry_item_t * item = (net_dns_entry_item_t *)it->data;
    it->next = net_dns_entry_address_it_next;
    *item = TAILQ_FIRST(&entry->m_items);
}

uint32_t net_dns_entry_hash(net_dns_entry_t o, void * user_data) {
    return cpe_hash_str(o->m_hostname, strlen(o->m_hostname));
}

int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r, void * user_data) {
    return strcmp(l->m_hostname, r->m_hostname) == 0 ? 1 : 0;
}
