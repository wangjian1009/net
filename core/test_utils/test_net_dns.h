#ifndef TEST_UTILS_NET_DNS_H_INCLEDED
#define TEST_UTILS_NET_DNS_H_INCLEDED
#include "test_net_driver.h"

typedef struct test_net_dns * test_net_dns_t;
typedef struct test_net_dns_record * test_net_dns_record_t;
typedef struct test_net_dns_query * test_net_dns_query_t;

typedef TAILQ_HEAD(test_net_dns_record_list, test_net_dns_record) test_net_dns_record_list_t;

struct test_net_dns {
    test_net_driver_t m_tdriver;
    test_net_dns_record_list_t m_records;
};

test_net_dns_t test_net_dns_create(test_net_driver_t tdriver);
void test_net_dns_free(test_net_dns_t resolver);

void test_net_dns_expect_query_cache(
    test_net_dns_t dns, const char * hostname, int64_t delay_ms);

void test_net_dns_expect_query_once(
    test_net_dns_t dns, const char * hostname, const char * responses, int64_t delay_ms);

#endif
