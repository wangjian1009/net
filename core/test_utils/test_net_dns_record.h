#ifndef TEST_UTILS_NET_DNS_RECORD_H_INCLEDED
#define TEST_UTILS_NET_DNS_RECORD_H_INCLEDED
#include "test_net_dns.h"

struct test_net_dns_record {
    test_net_dns_t m_dns;
    TAILQ_ENTRY(test_net_dns_record) m_next_for_dns;
    net_address_t m_target;
};

test_net_dns_record_t
test_net_dns_record_create(test_net_dns_t dns, net_address_t target);

void test_net_dns_record_free(test_net_dns_record_t record);

#endif
