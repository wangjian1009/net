#include <errno.h>
#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/file.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_address.h"
#include "net_dns_source_nameserver.h"
#include "net_dns_manage_i.h"

int net_dns_manage_load_resolv(net_dns_manage_t manage, const char * path);
int net_dns_manage_load_nameserver(net_dns_manage_t manage, char * str);

int net_dns_manage_auto_conf(net_dns_manage_t manage) {
    int rv = 0;

#if defined(ANDROID) || defined(__ANDROID__)
    unsigned int i;
    for (i = 1; i <= MAX_DNS_PROPERTIES; i++) {
        char propname[PROP_NAME_MAX];
        char propvalue[PROP_VALUE_MAX];
        snprintf(propname, sizeof(propname), "%s%u", DNS_PROP_NAME_PREFIX, i);
        if (__system_property_get(propname, propvalue) < 1) {
            CPE_ERROR(manage->m_em, "dns: get property %s fail!", propname);
            rv = -1;
            break;
        }
        if (net_dns_manage_load_nameserver(manage, propvalue) != 0) rv = -1;
    }
#else
    if (net_dns_manage_load_resolv(manage, "/etc/resolv.conf") != 0) rv = -1;
#endif
    
    return rv;
}

int net_dns_manage_load_resolv(net_dns_manage_t manage, const char * path) {
    FILE *fp = file_stream_open(path, "r", NULL);
    if (fp == NULL) {
        CPE_ERROR(
            manage->m_em, "dns: load resolv from %s: open fail!, errno=%d (%s)",
            path, errno, strerror(errno));
        return -1;
    }

    int rv = 0;
    char * line = NULL;
    size_t data_len = 0;

    mem_buffer_t buffer = net_dns_manage_tmp_buffer(manage);
    mem_buffer_clear_data(buffer);
    do {
        if (file_stream_read_line(buffer, &line, &data_len, fp, NULL) != 0) {
            CPE_ERROR(
                manage->m_em, "dns: load resolv from %s: read line fail, errno=%d (%s)",
                path, errno, strerror(errno));
            rv = -1;
            break;
        }

        if (line == NULL) break;

        if (cpe_str_start_with(line, "nameserver")) {
            if (net_dns_manage_load_nameserver(manage, cpe_str_trim_head(line + strlen("nameserver"))) != 0) rv = -1;
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

    file_stream_close(fp, manage->m_em);
    
    return rv;
}

int net_dns_manage_load_nameserver(net_dns_manage_t manage, char * str) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (sock_ip_init((struct sockaddr *)&addr, &addr_len, str, 0, NULL) != 0) {
        CPE_ERROR(manage->m_em, "dns: load nameserver: %s format error", str);
        return -1;
    }

    net_address_t address = net_address_create_from_sockaddr(manage->m_schedule, (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(manage->m_em, "dns: load nameserver: create address fail");
        return -1;
    }

    net_dns_source_nameserver_t nameserver = net_dns_source_nameserver_create(manage, address, 1);
    if (nameserver == NULL) {
        CPE_ERROR(manage->m_em, "dns: load nameserver: create nameserver fail");
        net_address_free(address);
        return -1;
    }

    return 0;
}
