#ifndef TEST_UTILS_NET_DNS_QUERY_H_INCLEDED
#define TEST_UTILS_NET_DNS_QUERY_H_INCLEDED
#include "test_net_dns.h"

enum test_net_dns_query_lifecycle {
    test_net_dns_query_lifecycle_mock,
    test_net_dns_query_lifecycle_noop,
};

struct test_net_dns_query {
    enum test_net_dns_query_lifecycle m_lifecycle;
    net_address_t m_hostname;
};

int test_net_dns_query_init(
    void * ctx, net_dns_query_t query, net_address_t hostname, net_dns_query_type_t query_type, const char * policy);

void test_net_dns_query_fini(void * ctx, net_dns_query_t query);

#endif
