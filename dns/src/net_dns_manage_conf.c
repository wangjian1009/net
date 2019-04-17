#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_address.h"
#include "net_dns_scope.h"
#include "net_dns_source.h"
#include "net_dns_source_ns.h"
#include "net_dns_manage_i.h"

static int net_dns_manage_load_ns(
    net_dns_manage_t manage, net_driver_t driver, net_dns_scope_t scope,
    net_address_t address, uint16_t timeout_s, uint16_t retry_count);

int net_dns_manage_auto_conf(net_dns_manage_t manage, net_driver_t driver, net_dns_scope_t scope, uint16_t timeout_s, uint16_t retry_count) {
    struct sockaddr_storage dnssevraddrs[10];
    uint8_t addr_count = CPE_ARRAY_SIZE(dnssevraddrs);
    if (getdnssvraddrs(dnssevraddrs, &addr_count, manage->m_em) != 0) return -1;

    int rv = 0;

    uint8_t i;
    for(i = 0; i < addr_count; ++i) {
        net_address_t address = net_address_create_from_sockaddr(manage->m_schedule, (struct sockaddr *)&dnssevraddrs[i], sizeof(dnssevraddrs[i]));
        if (address == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: load ns: create address from sock addr fail");
            rv = -1;
            continue;
        }

        if (net_dns_manage_load_ns(manage, driver, scope, address, timeout_s, retry_count) != 0) {
            rv = -1;
            continue;
        }
    }
    
    return rv;
}

static int net_dns_manage_load_ns(
    net_dns_manage_t manage, net_driver_t driver, net_dns_scope_t scope,
    net_address_t address, uint16_t timeout_s, uint16_t retry_count)
{
    if (net_dns_source_ns_find(manage, address) != NULL) {
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: load ns: ignore duplicate address %s",
                net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        }
        net_address_free(address);
        return 0;
    }
    
    net_dns_source_ns_t ns = net_dns_source_ns_create(manage, driver, address, 1, net_dns_trans_udp, timeout_s, retry_count);
    if (ns == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: load ns: create ns fail");
        net_address_free(address);
        return -1;
    }

    if (scope) {
        if (net_dns_scope_add_source(scope, net_dns_source_from_data(ns)) != 0) {
            CPE_ERROR(manage->m_em, "dns-cli: load ns: add to scope fail");
            net_dns_source_ns_free(ns);
            return -1;
        }
    }
    
    return 0;
}
