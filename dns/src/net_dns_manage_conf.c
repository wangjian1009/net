#include <errno.h>
#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/file.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_address.h"
#include "net_dns_source_ns.h"
#include "net_dns_manage_i.h"
#if defined __APPLE__
#define NET_DNS_USE_RESOLVE_H
#endif

#if defined NET_DNS_USE_RESOLVE_H
#include "resolv.h"
#endif

int net_dns_manage_load_resolv(net_dns_manage_t manage, net_driver_t driver, const char * path);
int net_dns_manage_load_ns_by_addr(net_dns_manage_t manage, net_driver_t driver, struct sockaddr * addr, socklen_t addr_len);
int net_dns_manage_load_ns_by_str(net_dns_manage_t manage, net_driver_t driver, char * str);

int net_dns_manage_auto_conf(net_dns_manage_t manage, net_driver_t driver) {
    int rv = 0;

#if defined(ANDROID) || defined(__ANDROID__)
    unsigned int i;
    for (i = 1; i <= MAX_DNS_PROPERTIES; i++) {
        char propname[PROP_NAME_MAX];
        char propvalue[PROP_VALUE_MAX];
        snprintf(propname, sizeof(propname), "%s%u", DNS_PROP_NAME_PREFIX, i);
        if (__system_property_get(propname, propvalue) < 1) {
            CPE_ERROR(manage->m_em, "dns-cli: get property %s fail!", propname);
            rv = -1;
            break;
        }
        if (net_dns_manage_load_ns_by_str(manage, propvalue) != 0) rv = -1;
    }
#else
# if defined (NET_DNS_USE_RESOLVE_H)
    struct __res_state res;
    if (res_ninit(&res) != 0) {
        CPE_ERROR(manage->m_em, "dns-cli: resolv: res_ninit fail!");
        rv = -1; 
    }
    else {
        int i;
        for(i = 0; i < res.nscount; ++i) {
            if (net_dns_manage_load_ns_by_addr(
                    manage, driver, (struct sockaddr *)&res.nsaddr_list[i], (socklen_t)sizeof(res.nsaddr_list[i])) != 0)
            {
                rv = -1;
            }
        }
    }
    
#else    
    if (net_dns_manage_load_resolv(manage, driver, "/etc/resolv.conf") != 0) rv = -1;
#endif
#endif

    return rv;
}

int net_dns_manage_load_resolv(net_dns_manage_t manage, net_driver_t driver, const char * path) {
    FILE *fp = file_stream_open(path, "r", NULL);
    if (fp == NULL) {
        CPE_ERROR(
            manage->m_em, "dns-cli: load resolv from %s: open fail!, errno=%d (%s)",
            path, errno, strerror(errno));
        return -1;
    }

    int rv = 0;
    char * line = NULL;
    size_t data_len = 0;

    struct mem_buffer buffer;
    mem_buffer_init(&buffer, manage->m_alloc);
    do {
        if (file_stream_read_line(&buffer, &line, &data_len, fp, NULL) != 0) {
            CPE_ERROR(
                manage->m_em, "dns-cli: load resolv from %s: read line fail, errno=%d (%s)",
                path, errno, strerror(errno));
            rv = -1;
            break;
        }

        if (line == NULL) break;

        if (cpe_str_start_with(line, "ns")) {
            if (net_dns_manage_load_ns_by_str(manage, driver, cpe_str_trim_head(line + strlen("ns"))) != 0) rv = -1;
        }
        else if (cpe_str_start_with(line, "domain")) {
        }
        else if (cpe_str_start_with(line, "lookup")) {
        }
        else if (cpe_str_start_with(line, "search")) {
        }
        else if (cpe_str_start_with(line, "sortlist")) {
        }
        else if (cpe_str_start_with(line, "options")) {
        }
    } while(1);

    mem_buffer_clear(&buffer);
    file_stream_close(fp, manage->m_em);
    
    return rv;
}

int net_dns_manage_load_ns(net_dns_manage_t manage, net_driver_t driver, net_address_t address) {
    if (net_dns_source_ns_find(manage, address) != NULL) {
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: load ns: ignore duplicate address %s",
                net_address_dump(net_dns_manage_tmp_buffer(manage), address));
        }
        net_address_free(address);
        return 0;
    }
    
    net_dns_source_ns_t ns = net_dns_source_ns_create(manage, driver, address, 1);
    if (ns == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: load ns: create ns fail");
        net_address_free(address);
        return -1;
    }

    return 0;
}

int net_dns_manage_load_ns_by_addr(net_dns_manage_t manage, net_driver_t driver, struct sockaddr * addr, socklen_t addr_len) {
    net_address_t address = net_address_create_from_sockaddr(manage->m_schedule, addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: load ns: create address from sock addr fail");
        return -1;
    }

    return net_dns_manage_load_ns(manage, driver, address);
}

int net_dns_manage_load_ns_by_str(net_dns_manage_t manage, net_driver_t driver, char * str) {
    net_address_t address = net_address_create_auto(manage->m_schedule, str);
    if (address == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: load ns: create address %s fail", str);
        return -1;
    }

    return net_dns_manage_load_ns(manage, driver, address);
}
