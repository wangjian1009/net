#include "cpe/pal/pal_types.h"
#include "cpe/pal/pal_string.h"
#include "net_address.h"
#include "net_dns_entry_i.h"

net_dns_entry_t
net_dns_entry_create(
    net_dns_manage_t manage, const char * hostname, net_address_t address, uint8_t is_own, uint32_t expire_time_ms)
{
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

    entry->m_hostname = entry->m_hostname_buf;
    memcpy(entry->m_hostname_buf, hostname, hostname_len);
    entry->m_expire_time_ms = expire_time_ms;

    if (is_own) {
        entry->m_address = address;
    }
    else {
        entry->m_address = net_address_copy(manage->m_schedule, address);
        if (entry->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns: entry dup address fail!");
            if (use_cache) {
                TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
            }
            else {
                mem_free(manage->m_alloc, entry);
            }
            return NULL;
        }
    }
    
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

void net_dns_entry_free(net_dns_manage_t manage, net_dns_entry_t entry) {
    size_t hostname_len = strlen(entry->m_hostname) + 1;
    uint8_t use_cache = hostname_len <= CPE_ENTRY_SIZE(net_dns_entry, m_hostname_buf);
    
    cpe_hash_table_remove_by_ins(&manage->m_entries, entry);

    if (use_cache) {
        TAILQ_INSERT_TAIL(&manage->m_free_entries, entry, m_next);
    }
    else {
        mem_free(manage->m_alloc, entry);
    }
}

void net_dns_entry_real_free(net_dns_manage_t manage, net_dns_entry_t entry) {
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
        net_dns_entry_free(manage, entry);
        entry = next;
    }
}

net_dns_entry_t
net_dns_entry_find(net_dns_manage_t manage, const char * hostname) {
    struct net_dns_entry key;
    key.m_hostname = hostname;
    return cpe_hash_table_find(&manage->m_entries, &key);
}

uint32_t net_dns_entry_hash(net_dns_entry_t o) {
    return cpe_hash_str(o->m_hostname, strlen(o->m_hostname));
}

int net_dns_entry_eq(net_dns_entry_t l, net_dns_entry_t r) {
    return strcmp(l->m_hostname, r->m_hostname) == 0 ? 1 : 0;
}
