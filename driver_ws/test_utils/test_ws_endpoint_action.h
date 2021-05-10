#ifndef TEST_UTILS_WS_ENDPOINT_ACTION_H_INCLEDED
#define TEST_UTILS_WS_ENDPOINT_ACTION_H_INCLEDED
#include "test_net_driver.h"
#include "net_ws_endpoint.h"

typedef enum test_net_ws_endpoint_action_type test_net_ws_endpoint_action_type_t;
typedef struct test_net_ws_endpoint_action * test_net_ws_endpoint_action_t;

enum test_net_ws_endpoint_action_type {
    test_net_ws_endpoint_op_noop,
    test_net_ws_endpoint_op_disable,
    test_net_ws_endpoint_op_error,
    test_net_ws_endpoint_op_delete,
    test_net_ws_endpoint_op_close,
};

struct test_net_ws_endpoint_action {
    test_net_ws_endpoint_action_type_t m_type;
    union {
        struct {
            uint16_t m_status_code;
            const char * m_msg;
        } m_close;
    };
};

void test_net_ws_endpoint_apply_action(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint,
    test_net_ws_endpoint_action_t action, int64_t delay_ms);

#endif
