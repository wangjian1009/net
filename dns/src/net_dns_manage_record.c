#include <assert.h>
#include "cpe/utils/time_utils.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_dns_manage_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_entry_alias_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_source_ns_i.h"

int net_dns_manage_add_record(net_dns_manage_t manage, net_address_t hostname, net_address_t address, uint32_t ttl_s) {
    assert(hostname);
    assert(net_address_type(hostname) == net_address_domain);
    assert(address);

    net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
    if (entry == NULL) {
        entry = net_dns_entry_create(manage, hostname);
        if (entry == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: add record %s ==> %s: entry create fail",
                (const char *)net_address_data(hostname),
                net_address_host(net_dns_manage_tmp_buffer(manage), address));
        }
        return -1;
    }

    uint32_t cur_time_s = (uint32_t)(cur_time_ms() / 1000);
    if (entry->m_expire_time_s) {
        if (entry->m_expire_time_s < cur_time_s) {
            if (manage->m_debug) {
                CPE_INFO(
                    manage->m_em, "dns-cli: entry %s expired, clear old, ttl=%d(s)",
                    (const char *)net_address_data(hostname), ttl_s);
            }
            net_dns_entry_clear(entry);
            entry->m_expire_time_s = cur_time_s + ttl_s;
        }
    }
    else {
        if (ttl_s) {
            if (manage->m_debug) {
                CPE_INFO(
                    manage->m_em, "dns-cli: entry %s received, ttl=%d(s)",
                    (const char *)net_address_data(hostname), ttl_s);
            }
            entry->m_expire_time_s = cur_time_s + ttl_s;
        }
    }
    
    net_dns_entry_item_t item = net_dns_entry_item_find(entry, address);
    if (item) {
        /* if (manage->m_debug) { */
        /*     CPE_INFO( */
        /*         manage->m_em, "dns-cli: updated %s ==> %s", */
        /*         (const char *)net_address_data(entry->m_hostname), */
        /*         item->m_address ? net_address_host(net_dns_manage_tmp_buffer(manage), item->m_address) : "N/A"); */
        /* } */
    }
    else {
        item = net_dns_entry_item_create(entry, address, 0);
        if (item == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: add record %s ==> %s create item fail",
                (const char *)net_address_data(hostname),
                net_address_host(net_dns_manage_tmp_buffer(manage), address));
            return -1;
        }

        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: resolved %s ==> %s",
                (const char *)net_address_data(entry->m_hostname),
                item->m_address ? net_address_host(net_dns_manage_tmp_buffer(manage), item->m_address) : "N/A");
        }
        
        if (net_address_type(address) == net_address_domain) {
            net_dns_entry_t cname_entry = net_dns_entry_find(manage, address);
            if (cname_entry == NULL) {
                cname_entry = net_dns_entry_create(manage, address);
                if (cname_entry == NULL) {
                    CPE_ERROR(
                        manage->m_em, "dns-cli: add record %s ==> %s create cname entry fail",
                        (const char *)net_address_data(hostname),
                        net_address_host(net_dns_manage_tmp_buffer(manage), address));
                    net_dns_entry_item_free(item);
                    return -1;
                }
            }

            if (net_dns_entry_is_origin_of(entry, cname_entry)) {
                /*ok*/
            }
            else {
                if (net_dns_entry_is_origin_of(cname_entry, entry)) {
                    CPE_ERROR(
                        manage->m_em, "dns-cli: add record %s ==> %s: cname circle",
                        (const char *)net_address_data(entry->m_hostname),
                        (const char *)net_address_data(cname_entry->m_hostname));
                    return -1;
                }
                else {
                    if (net_dns_entry_alias_create(entry, cname_entry) == NULL) {
                        CPE_ERROR(
                            manage->m_em, "dns-cli: add record %s ==> %s: create alais fail",
                            (const char *)net_address_data(entry->m_hostname),
                            (const char *)net_address_data(cname_entry->m_hostname));
                        return -1;
                    }
                }
            }
        }
    }
    
    return 0;
}

