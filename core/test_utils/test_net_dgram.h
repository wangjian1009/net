#ifndef TEST_UTILS_NET_DGRAM_H_INCLEDED
#define TEST_UTILS_NET_DGRAM_H_INCLEDED
#include "net_dgram.h"
#include "test_net_driver.h"

typedef enum test_net_dgram_write_policy_type {
    test_net_dgram_write_mock,
    test_net_dgram_write_remove,
    test_net_dgram_write_send,
} test_net_dgram_write_policy_type_t;

struct test_net_dgram_write_policy {
    test_net_dgram_write_policy_type_t m_type;
    union {
        struct {
            int64_t m_write_delay_ms;
        } m_send;
    };
};

struct test_net_dgram {
    TAILQ_ENTRY(test_net_dgram) m_next;
    struct test_net_dgram_write_policy m_write_policy;
};

int test_net_dgram_init(net_dgram_t base_dgram);
void test_net_dgram_fini(net_dgram_t base_dgram);
int test_net_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

void test_net_dgram_expect_write_remove(net_dgram_t base_dgram);
void test_net_dgram_expect_write_send(net_dgram_t base_dgram, int64_t delay_ms);

#endif
