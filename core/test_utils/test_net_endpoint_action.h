#ifndef TEST_UTILS_NET_ENDPOINT_ACTION_H_INCLEDED
#define TEST_UTILS_NET_ENDPOINT_ACTION_H_INCLEDED
#include "net_endpoint.h"
#include "test_net_driver.h"

typedef enum test_net_endpoint_action_type test_net_endpoint_action_type_t;
typedef struct test_net_endpoint_action * test_net_endpoint_action_t;

enum test_net_endpoint_action_type {
    test_net_endpoint_action_noop,
    test_net_endpoint_action_buf_copy,
    test_net_endpoint_action_buf_erase,
    test_net_endpoint_action_buf_link,
    test_net_endpoint_action_error,
    test_net_endpoint_action_disable,
    test_net_endpoint_action_delete,
};

struct test_net_endpoint_action {
    test_net_endpoint_action_type_t m_type;
    union {
        struct {
            net_endpoint_buf_type_t m_buf_type;
        } m_buf_copy;
        struct {
            net_endpoint_error_source_t m_error_source;
            int m_error_no;
            char * m_error_msg;
        } m_error;
    };
};

void test_net_endpoint_apply_action(
    test_net_driver_t tdriver, net_endpoint_t endpoint,
    test_net_endpoint_action_t action,
    int64_t delay_ms);

#endif
