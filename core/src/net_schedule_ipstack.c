#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils_sock/getgateway.h"
#include "cpe/utils_sock/getdnssvraddrs.h"
#include "net_schedule_i.h"
#include "net_local_ip_stack_monitor_i.h"

static uint8_t net_schedule_local_ip_stack_have_ipv6(net_schedule_t schedule);
static uint8_t net_schedule_local_ip_stack_have_ipv4(net_schedule_t schedule);

net_local_ip_stack_t net_schedule_local_ip_stack(net_schedule_t schedule) {
    return schedule->m_local_ip_stack;
}

void net_schedule_set_local_ip_stack(net_schedule_t schedule, net_local_ip_stack_t local_ip_stack) {
    if (schedule->m_local_ip_stack == local_ip_stack) return;
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "core: network %s ==> %s", 
            net_local_ip_stack_str(schedule->m_local_ip_stack), net_local_ip_stack_str(local_ip_stack));
    }

    schedule->m_local_ip_stack = local_ip_stack;
    
    net_local_ip_stack_monitor_t monitor = TAILQ_FIRST(&schedule->m_local_ip_stack_monitors);
    while(monitor) {
        uint8_t tag_local = 0;
        if (!monitor->m_is_processing) {
            tag_local = 1;
            monitor->m_is_processing = 1;
        }
        
        monitor->m_on_change(monitor->m_ctx, schedule);

        net_local_ip_stack_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next);
            
        if (tag_local) {
            monitor->m_is_processing = 0;
            if (monitor->m_is_free) net_local_ip_stack_monitor_free(monitor);
        }
        
        monitor = next_monitor;
    }
}

int net_schedule_local_ip_stack_detect(net_schedule_t schedule) {
    struct in6_addr addr6_gateway = IN6ADDR_ANY_INIT;
    if (0 != getdefaultgateway6(&addr6_gateway) || IN6_IS_ADDR_UNSPECIFIED(&addr6_gateway)) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "core: ip stack detect: no ipv6 defaultgetway");
        }
        net_schedule_set_local_ip_stack(schedule, net_local_ip_stack_ipv4);
        return 0;
    }
    
    struct in_addr addr_gateway = {0};
    if (0 != getdefaultgateway(&addr_gateway) || INADDR_NONE == addr_gateway.s_addr || INADDR_ANY == addr_gateway.s_addr) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "core: ip stack detect: no ipv4 defaultgetway");
        }
        net_schedule_set_local_ip_stack(schedule, net_local_ip_stack_ipv6);
        return 0;
    }
    
    uint8_t have_ipv4 = net_schedule_local_ip_stack_have_ipv4(schedule);
    uint8_t have_ipv6 = net_schedule_local_ip_stack_have_ipv6(schedule);
    int local_stack = 0;
    if (have_ipv4) { local_stack |= net_local_ip_stack_ipv4; }
    if (have_ipv6) { local_stack |= net_local_ip_stack_ipv6; }
    if (net_local_ip_stack_dual != local_stack) {
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "core: ip stack detect: ipv4 test %s, ipv6 test %s",
                have_ipv4 ? "success" : "fail", have_ipv6 ? "success" : "fail");
        }

        net_schedule_set_local_ip_stack(schedule, (net_local_ip_stack_t)local_stack);
        return 0;
    }
    
    int dns_ip_stack = 0;
    struct sockaddr_storage dnsaddrs[5];
    uint8_t dnsaddrs_count = CPE_ARRAY_SIZE(dnsaddrs);
    getdnssvraddrs(dnsaddrs, &dnsaddrs_count, schedule->m_em);
    int i;
    for (i = 0; i < dnsaddrs_count; ++i) {
        if (AF_INET == dnsaddrs[i].ss_family) {
            dns_ip_stack |= net_local_ip_stack_ipv4;
        }

        if (AF_INET6 == dnsaddrs[i].ss_family) {
            dns_ip_stack |= net_local_ip_stack_ipv6;
        }
    }

    if (dns_ip_stack != net_local_ip_stack_none) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "core: ip stack detect: dns tested");
        }

        net_schedule_set_local_ip_stack(schedule, (net_local_ip_stack_t)local_stack);
        return 0;
    }
    else {
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "core: ip stack detect: no dns tested, ipv4 test %s, ipv6 test %s",
                have_ipv4 ? "success" : "fail", have_ipv6 ? "success" : "fail");
        }

        net_schedule_set_local_ip_stack(schedule, (net_local_ip_stack_t)local_stack);
        return 0;
    }
}

const char * net_local_ip_stack_str(net_local_ip_stack_t ipstack) {
    switch(ipstack) {
    case net_local_ip_stack_none:
        return "ip-stack-none";
    case net_local_ip_stack_ipv4:
        return "ip-stack-ipv4";
    case net_local_ip_stack_ipv6:
        return "ip-stack-ipv6";
    case net_local_ip_stack_dual:
        return "ip-stack-dual";
    }
}

//Android的AI_ADDRCONFIG 功能的sample
static uint8_t net_schedule_local_ip_stack_test_connect(net_schedule_t schedule, int pf, struct sockaddr *addr, socklen_t addrlen) {
    int s = socket(pf, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) return 0;

    int ret;
    do {
        ret = connect(s, addr, addrlen);
    } while (ret < 0 && errno == EINTR);

    if (ret != 0 && schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "net_schedule_local_ip_stack_test_connect: errno=%d, %s", errno, strerror(errno));
    }

    int success = (ret == 0);
    do {
        ret = close(s);
    } while (ret < 0 && errno == EINTR);

    return success;
}

static uint8_t net_schedule_local_ip_stack_have_ipv6(net_schedule_t schedule) {
    struct sockaddr_in6 sin6_test = {
#if defined __APPLE__
        .sin6_len = sizeof(struct sockaddr_in6),
#endif
        .sin6_family = AF_INET6,
        .sin6_port = htons(0xFFFF),
        .sin6_addr.s6_addr = {
            0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    return net_schedule_local_ip_stack_test_connect(schedule, PF_INET6, (struct sockaddr *)&sin6_test, sizeof(sin6_test));
}

static uint8_t net_schedule_local_ip_stack_have_ipv4(net_schedule_t schedule) {
    struct sockaddr_in sin_test = {
#if defined __APPLE__
        .sin_len = sizeof(struct sockaddr_in),
#endif
        .sin_family = AF_INET,
        .sin_port = htons(0xFFFF),
        .sin_addr.s_addr = htonl(0x08080808L),  // 8.8.8.8
    };
    return net_schedule_local_ip_stack_test_connect(schedule, PF_INET, (struct sockaddr *)&sin_test, sizeof(sin_test));
}
