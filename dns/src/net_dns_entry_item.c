#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_source_i.h"

net_dns_entry_item_t
net_dns_entry_item_create(
    net_dns_entry_t entry, net_dns_source_t source,
    net_address_t address, uint8_t is_own,
    uint32_t expire_time_s)
{
    net_dns_manage_t manage = entry->m_manage;

    assert(address);

    net_dns_entry_item_t item = TAILQ_FIRST(&manage->m_free_entry_items);
    if (item) {
        TAILQ_REMOVE(&manage->m_free_entry_items, item, m_next_for_entry);
    }
    else {
        item = mem_alloc(manage->m_alloc, sizeof(struct net_dns_entry_item));
        if (item == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: entry item alloc fail");
            return NULL;
        }
    }

    item->m_entry = entry;
    item->m_source = source;
    item->m_expire_time_s = expire_time_s;

    if (is_own) {
        item->m_address = address;
    }
    else {
        item->m_address = net_address_copy(manage->m_schedule, address);
        if (item->m_address == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: entry item dup address fail!");
            item->m_entry = (net_dns_entry_t)manage;
            TAILQ_INSERT_TAIL(&manage->m_free_entry_items, item, m_next_for_entry);
            return NULL;
        }
    }

    TAILQ_INSERT_TAIL(&entry->m_items, item, m_next_for_entry);
    TAILQ_INSERT_TAIL(&source->m_items, item, m_next_for_source);
    
    if (manage->m_debug) {
        char source_buf[64];
        cpe_str_dup(source_buf, sizeof(source_buf), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), item->m_source));
        
        CPE_INFO(
            manage->m_em, "dns-cli: resolved %s ==> %s | %s",
            entry->m_hostname,
            item->m_address ? net_address_host(net_dns_manage_tmp_buffer(manage), item->m_address) : "N/A",
            source_buf);
    }

    return item;
}
        
void net_dns_entry_item_free(net_dns_entry_item_t item) {
    net_dns_entry_t entry = item->m_entry;
    net_dns_manage_t manage = entry->m_manage;

    if (manage->m_debug) {
        CPE_INFO(
            manage->m_em, "dns-cli: removed %s ==> %s",
            entry->m_hostname,
            net_address_host(net_dns_manage_tmp_buffer(manage), item->m_address));
    }

    net_address_free(item->m_address);
    item->m_address = NULL;

    if (item->m_source) {
        TAILQ_REMOVE(&item->m_source->m_items, item, m_next_for_source);
    }
    
    TAILQ_REMOVE(&entry->m_items, item, m_next_for_entry);

    item->m_entry = (net_dns_entry_t)entry->m_manage;
    TAILQ_INSERT_TAIL(&entry->m_manage->m_free_entry_items, item, m_next_for_entry);
}

net_dns_entry_item_t
net_dns_entry_item_find(net_dns_entry_t entry, net_dns_source_t source, net_address_t address) {
    net_dns_entry_item_t item;

    TAILQ_FOREACH(item, &entry->m_items, m_next_for_entry) {
        if (item->m_source == source && net_address_cmp(item->m_address, address) == 0) return item;
    }
    
    return NULL;
}

void net_dns_entry_item_real_free(net_dns_entry_item_t item) {
    net_dns_manage_t manage = (net_dns_manage_t)item->m_entry;
    TAILQ_REMOVE(&manage->m_free_entry_items, item, m_next_for_entry);
    mem_free(manage->m_alloc, item);
}

net_dns_entry_t net_dns_entry_item_entry(net_dns_entry_item_t item) {
    return item->m_entry;
}

net_dns_source_t net_dns_entry_item_source(net_dns_entry_item_t item) {
    return item->m_source;
}

net_address_t net_dns_entry_item_address(net_dns_entry_item_t item) {
    return item->m_address;
}
