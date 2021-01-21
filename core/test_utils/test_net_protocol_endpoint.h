#ifndef TEST_UTILS_NET_PROTOCOL_ENDPOINT_H_INCLEDED
#define TEST_UTILS_NET_PROTOCOL_ENDPOINT_H_INCLEDED
#include "net_protocol.h"

typedef struct test_net_protocol_endpoint * test_net_protocol_endpoint_t;
typedef struct test_net_protocol_endpoint_link * test_net_protocol_endpoint_link_t;

typedef enum test_net_protocol_endpoint_input_policy_type {
    test_net_protocol_endpoint_input_mock,
    test_net_protocol_endpoint_input_keep,
    test_net_protocol_endpoint_input_remove,
    test_net_protocol_endpoint_input_link,
} test_net_protocol_endpoint_input_policy_type_t;

struct test_net_protocol_endpoint_input_policy {
    test_net_protocol_endpoint_input_policy_type_t m_type;
    union {
        struct {
            net_endpoint_buf_type_t m_buf_type;
        } m_keep;
        struct {
            test_net_protocol_endpoint_link_t m_link;
            int64_t m_input_delay_ms;
        } m_link;
    };
};

struct test_net_protocol_endpoint {
    struct test_net_protocol_endpoint_input_policy m_input_policy;
};

void test_net_protocol_endpoint_input_policy_clear(test_net_protocol_endpoint_t base_endpoint);

void test_net_protocol_endpoint_expect_input_keep(
    net_endpoint_t base_endpoint, net_endpoint_buf_type_t buf_type);

net_protocol_t test_net_protocol_create(net_schedule_t schedule, const char * name);

#endif
