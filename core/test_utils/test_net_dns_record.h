#ifndef TEST_UTILS_NET_DNS_RECORD_H_INCLEDED
#define TEST_UTILS_NET_DNS_RECORD_H_INCLEDED
#include "test_net_dns.h"

struct test_net_dns_record {
    test_net_dns_t m_dns;
    TAILQ_ENTRY(test_net_dns_record) m_next_for_dns;
    net_address_t m_hostname;
    uint16_t m_resolved_count;
    uint16_t m_resolved_capacity;
    net_address_t * m_resolveds;
};

test_net_dns_record_t
test_net_dns_record_create(test_net_dns_t dns, net_address_t hostname);

void test_net_dns_record_free(test_net_dns_record_t record);

test_net_dns_record_t
test_net_dns_record_find(test_net_dns_t dns, net_address_t hostname);

uint8_t test_net_dns_record_ins_in_resolved(test_net_dns_record_t record, net_address_t resolved);

int test_net_dns_record_add_resolved(test_net_dns_record_t record, net_address_t resolved);

void test_net_dns_record_resolveds(test_net_dns_record_t record, net_address_it_t resolveds);

#endif
