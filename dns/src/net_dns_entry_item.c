#include "assert.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_source_i.h"

net_dns_entry_item_t
net_dns_entry_item_create(net_dns_entry_t entry, net_address_t address, uint8_t is_own) {
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

    TAILQ_REMOVE(&entry->m_items, item, m_next_for_entry);

    item->m_entry = (net_dns_entry_t)entry->m_manage;
    TAILQ_INSERT_TAIL(&entry->m_manage->m_free_entry_items, item, m_next_for_entry);
}

net_dns_entry_item_t
net_dns_entry_item_find(net_dns_entry_t entry, net_address_t address) {
    net_dns_entry_item_t item;

    TAILQ_FOREACH(item, &entry->m_items, m_next_for_entry) {
        if (net_address_cmp_without_port(item->m_address, address) == 0) return item;
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

net_address_t net_dns_entry_item_address(net_dns_entry_item_t item) {
    return item->m_address;
}
