#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_address_rule_i.h"
#include "net_schedule_i.h"

uint8_t net_schedule_is_domain_address_valid(net_schedule_t schedule, const char * str_address) {
    if (schedule->m_domain_address_rule == NULL) {
        schedule->m_domain_address_rule =
            net_address_rule_create(
                schedule,
                "[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+\\.?");
        if (schedule->m_domain_address_rule == NULL) {
            return 0;
        }
    }

    assert(schedule->m_domain_address_rule);
    return net_address_rule_check(schedule, schedule->m_domain_address_rule, str_address);
}

uint8_t net_schedule_is_domain_address_arpa(net_schedule_t schedule, const char * str_address, net_address_t * address) {
    const char * end = str_address + strlen(str_address);
    const char * p = cpe_str_rchr_range(str_address, end, '.');
    if (p == NULL) return 0;

    if (p + 1 == end) {
        end = p;
        p = cpe_str_rchr_range(str_address, end, '.');
        if (p == NULL) return 0;
    }

    if (end - p != 5 || memcmp(p, ".arpa", 5) != 0) return 0;

    end = p;
    p = cpe_str_rchr_range(str_address, end, '.');
    if (p == NULL) return 0;

    if (end - p == 8 && memcmp(p, ".in-addr", 8) == 0) {
        const char * p4 = p;

        const char * p3 = cpe_str_rchr_range(str_address, p4, '.');
        if (p3 == NULL) return 0;

        const char * p2 = cpe_str_rchr_range(str_address, p3, '.');
        if (p2 == NULL) return 0;

        const char * p1 = cpe_str_rchr_range(str_address, p2, '.');
        if (p1 == NULL) return 0;

        if (cpe_str_rchr_range(str_address, p1, '.') != NULL) return 0;

        if (address) {
            struct net_address_data_ipv4 addr_data;
            addr_data.u8[0] = atoi(str_address);
            addr_data.u8[1] = atoi(p1 + 1);
            addr_data.u8[2] = atoi(p2 + 1);
            addr_data.u8[3] = atoi(p3 + 1);
            *address = net_address_create_ipv4_from_data(schedule, &addr_data, 0);
        }
        
        return 1;
    }
    else if (end - p == 4 && memcmp(p, ".ip6", 4) == 0) {
        if (address) *address = NULL;
        return 1;
    }
    else {
        return 0;
    }
}
