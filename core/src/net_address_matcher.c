#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_ipset.h"
#include "net_address.h"
#include "net_address_matcher_i.h"
#include "net_address_rule_i.h"

net_address_matcher_t net_address_matcher_create(net_schedule_t schedule) {
    net_address_matcher_t matcher = mem_alloc(schedule->m_alloc, sizeof(struct net_address_matcher));
    if (matcher == NULL) {
        CPE_ERROR(schedule->m_em, "net_address_matcher: alloc fail!");
        return NULL;
    }

    matcher->m_schedule = schedule;

    matcher->m_ipset_ipv4 = net_ipset_create(schedule);
    if (matcher->m_ipset_ipv4 == NULL) {
        CPE_ERROR(schedule->m_em, "net_address_matcher: create ipset(ipv4) fail !");
        mem_free(schedule->m_alloc, matcher);
        return NULL;
    }
    
    matcher->m_ipset_ipv6 = net_ipset_create(schedule);
    if (matcher->m_ipset_ipv4 == NULL) {
        CPE_ERROR(schedule->m_em, "net_address_matcher: create ipset(ipv6) fail !");
        net_ipset_free(matcher->m_ipset_ipv4);
        mem_free(schedule->m_alloc, matcher);
        return NULL;
    }

    TAILQ_INIT(&matcher->m_rules);
    
    return matcher;
}

void net_address_matcher_free(net_address_matcher_t matcher) {
    net_ipset_free(matcher->m_ipset_ipv4);
    net_ipset_free(matcher->m_ipset_ipv6);

    while(!TAILQ_EMPTY(&matcher->m_rules)) {
        net_address_rule_t rule = TAILQ_FIRST(&matcher->m_rules);
        TAILQ_REMOVE(&matcher->m_rules, rule, m_next);
        net_address_rule_free(matcher->m_schedule, rule);
    }
    
    mem_free(matcher->m_schedule->m_alloc, matcher);
}

uint8_t net_address_matcher_match(net_address_matcher_t matcher, net_address_t address) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_contains_ipv4(matcher->m_ipset_ipv4, (net_address_data_ipv4_t)net_address_data(address));
    case net_address_ipv6:
        return net_ipset_contains_ipv6(matcher->m_ipset_ipv6, (net_address_data_ipv6_t)net_address_data(address));
    case net_address_domain:
        return net_address_rule_lookup(matcher, net_address_domain_url(address)) ? 1 : 0;
    case net_address_local:
        return net_address_rule_lookup(matcher, net_address_local_path(address)) ? 1 : 0;
    }
}

int net_address_matcher_add(net_address_matcher_t matcher, net_address_t address) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        net_ipset_ipv4_add(
            matcher->m_ipset_ipv4,
            (net_address_data_ipv4_t)net_address_data(address));
        return 0;
    case net_address_ipv6:
        net_ipset_ipv6_add(
            matcher->m_ipset_ipv6,
            (net_address_data_ipv6_t)net_address_data(address));
        return 0;
    case net_address_domain: {
        net_schedule_t schedule = matcher->m_schedule;

        char buf[256];
        char * p = buf;
        char * e = p + sizeof(buf) - 1;

        *p++ = '^';

        const char * input = (const char *)net_address_data(address);
        while(*input && p < e) {
            char c = *input++;
            if (c == '.') {
                if (e - p < 2) {
                    CPE_ERROR(
                        schedule->m_em,
                        "net_address_matcher_add: create rule %s: overflow!", (const char *)net_address_data(address));
                    return -1;
                }
                *p++ = '\\';
                *p++ = c;
            }
            else {
                *p++ = c;
            }
        }

        if (e - p < 2) {
            CPE_ERROR(
                schedule->m_em,
                "net_address_matcher_add: create rule %s: overflow!", (const char *)net_address_data(address));
            return -1;
        }

        *p++ = '$';
        *p++ = 0;

        if (net_address_matcher_add_rule(matcher, buf) == NULL) {
            CPE_ERROR(
                schedule->m_em,
                "net_address_matcher_add: create rule %s fail!", (const char *)net_address_data(address));
            return -1;
        }
        return 0;
    }
    case net_address_local:
        CPE_ERROR(
            matcher->m_schedule->m_em,
            "net_address_matcher_add: not support local address!");
        return -1;
    }
}

int net_address_matcher_add_network(net_address_matcher_t matcher, net_address_t address, uint8_t cidr) {
    switch(net_address_type(address)) {
    case net_address_ipv4:
        return net_ipset_ipv4_add_network(
            matcher->m_ipset_ipv4,
            (net_address_data_ipv4_t)net_address_data(address),
            cidr)
            == 0 ? 0 : -1;
    case net_address_ipv6:
        return net_ipset_ipv6_add_network(
            matcher->m_ipset_ipv6,
            (net_address_data_ipv6_t)net_address_data(address),
            cidr)
            == 0 ? 0 : -1;
    case net_address_domain:
    case net_address_local:
        CPE_ERROR(
            matcher->m_schedule->m_em,
            "net_address_matcher_add_network: not support %s!",
            net_address_type_str(net_address_type(address)));
        return -1;
    }
}

net_address_rule_t net_address_matcher_add_rule(net_address_matcher_t matcher, const char * str_rule) {
    net_address_rule_t rule = net_address_rule_create(matcher->m_schedule, str_rule);
    if (rule) {
        TAILQ_INSERT_TAIL(&matcher->m_rules, rule, m_next);
    }
    return rule;
}

static const char * net_address_matcher_parse_cidr(mem_buffer_t buffer, const char * str, uint8_t * cidr) {
    const char * p = strrchr(str, '/');
    if (p == NULL) {
        *cidr = -1;
        return str;
    }
    else {
        *cidr = atoi(p + 1);
        mem_buffer_clear_data(buffer);
        return mem_buffer_strdup_len(buffer, str, p - str);
    }
}

int net_address_matcher_add_by_string(net_address_matcher_t matcher, const char * def) {
    uint8_t cidr;
    const char * host = net_address_matcher_parse_cidr(&matcher->m_schedule->m_tmp_buffer, def, &cidr);

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (sock_ip_init((struct sockaddr *)&addr, &addr_len, host, 0, NULL) == 0) {
        if (((struct sockaddr *)&addr)->sa_family == AF_INET) {
            if (cidr == (uint8_t)-1) {
                return net_ipset_ipv4_add(
                    matcher->m_ipset_ipv4,
                    (net_address_data_ipv4_t)(void*)&((struct sockaddr_in *)&addr)->sin_addr)
                    == 0 ? 0 : -1;
            }
            else {
                return net_ipset_ipv4_add_network(
                    matcher->m_ipset_ipv4,
                    (net_address_data_ipv4_t)(void*)&((struct sockaddr_in *)&addr)->sin_addr,
                    cidr)
                    == 0 ? 0 : -1;
            }
        }
        else if (((struct sockaddr *)&addr)->sa_family == AF_INET6) {
            if (cidr == (uint8_t)-1) {
                return net_ipset_ipv6_add(
                    matcher->m_ipset_ipv6,
                    (net_address_data_ipv6_t)(void*)&((struct sockaddr_in6 *)&addr)->sin6_addr)
                    == 0 ? 0 : -1;
            }
            else {
                return net_ipset_ipv6_add_network(
                    matcher->m_ipset_ipv6,
                    (net_address_data_ipv6_t)(void*)&((struct sockaddr_in6 *)&addr)->sin6_addr,
                    cidr) == 0 ? 0 : -1;
            }
        }
        else { 
            CPE_ERROR(
                matcher->m_schedule->m_em,
                "net_address_matcher_add_by_string: not support address type!");
            return -1;
        }
    }
    else {
        net_address_rule_t rule = net_address_matcher_add_rule(matcher, def);
        if (rule == NULL) {
            CPE_ERROR(
                matcher->m_schedule->m_em,
                "net_address_matcher_add_by_string: create rule %s fail!", def);
            return -1;
        }

        return 0;
    }
}

net_address_rule_t
net_address_rule_lookup(net_address_matcher_t matcher, const char * address) {
    net_address_rule_t rule;

    TAILQ_FOREACH(rule, &matcher->m_rules, m_next) {
        if (net_address_rule_check(matcher->m_schedule, rule, address)) {
            return rule;
        }
    }

    return NULL;
}
