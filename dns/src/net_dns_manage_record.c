#include "cpe/utils/time_utils.h"
#include "net_address.h"
#include "net_dns_manage_i.h"
#include "net_dns_entry_i.h"
#include "net_dns_entry_item_i.h"
#include "net_dns_entry_alias_i.h"
#include "net_dns_task_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_source_ns_i.h"

int net_dns_manage_add_record(
    net_dns_manage_t manage, net_dns_source_t source, uint16_t task_ident, 
    const char * hostname, net_address_t address, uint32_t ttl)
{
    net_dns_entry_t entry = net_dns_entry_find(manage, hostname);
    if (entry == NULL) {
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: add record %s ==> %s ttl=%d, entry not exist",
                hostname,
                net_address_host(net_dns_manage_tmp_buffer(manage), address),
                ttl);
        }
        return -1;
    }

    uint32_t expire_time_s = ttl ? ((uint32_t)(cur_time_ms() / 1000) + ttl) : 0;
    
    net_dns_entry_item_t item = net_dns_entry_item_find(entry, source, address);
    if (item) {
        CPE_ERROR(
            manage->m_em, "xxxxx: dns-cli: %s update %s", entry->m_hostname,
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        
        item->m_expire_time_s = expire_time_s;
    }
    else {
        item = net_dns_entry_item_create(entry, source, address, 0, expire_time_s);
        if (item == NULL) {
            CPE_ERROR(
                manage->m_em, "dns-cli: add record %s ==> %s ttl=%d, create item fail",
                hostname,
                net_address_host(net_dns_manage_tmp_buffer(manage), address),
                ttl);
            return -1;
        }

        CPE_ERROR(
            manage->m_em, "xxxxx: dns-cli: %s add %s", entry->m_hostname,
            net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        
        if (net_address_type(address) == net_address_domain) {
            const char * cname = net_address_data(address);
            net_dns_entry_t cname_entry = net_dns_entry_find(manage, cname);
            if (cname_entry == NULL) {
                cname_entry = net_dns_entry_create(manage, cname);
                if (cname_entry == NULL) {
                    CPE_ERROR(
                        manage->m_em, "dns-cli: add record %s ==> %s ttl=%d, create cname entry fail",
                        hostname,
                        net_address_host(net_dns_manage_tmp_buffer(manage), address),
                        ttl);
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
                        entry->m_hostname, cname_entry->m_hostname);
                    return -1;
                }
                else {
                    if (net_dns_entry_alias_create(entry, cname_entry) == NULL) {
                        CPE_ERROR(
                            manage->m_em, "dns-cli: add record %s ==> %s: create alais fail",
                            entry->m_hostname, cname_entry->m_hostname);
                        return -1;
                    }
                }
            }
        }
    }
    
    return 0;
}

