#ifndef TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#define TEST_UTILS_WS_ENDPOINT_RECEIVER_H_INCLEDED
#include "test_net_driver.h"
#include "net_ws_endpoint.h"

typedef enum test_net_ws_endpoint_action_type test_net_ws_endpoint_action_type_t;
typedef struct test_net_ws_endpoint_action * test_net_ws_endpoint_action_t;

enum test_net_ws_endpoint_action_type {
    test_net_ws_endpoint_op_noop,
    test_net_ws_endpoint_op_disable,
    test_net_ws_endpoint_op_error,
    test_net_ws_endpoint_op_delete,
};

struct test_net_ws_endpoint_action {
    test_net_ws_endpoint_action_type_t m_type;
};

/*安装测试桩 */
void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint);

/*设置函数 */
void test_net_ws_endpoint_expect_text_msg(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, const char * msg,
    test_net_ws_endpoint_action_t action, int64_t delay_ms);

void test_net_ws_endpoint_expect_bin_msg(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len,
    test_net_ws_endpoint_action_t action, int64_t delay_ms);

void test_net_ws_endpoint_expect_close(
    test_net_driver_t tdriver, net_ws_endpoint_t endpoint, uint16_t status_code, const char * msg,
    test_net_ws_endpoint_action_t action, int64_t delay_ms);

#endif
