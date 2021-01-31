#include "cmocka_all.h"
#include "net_endpoint.h"
#include "test_ws_endpoint_receiver.h"

typedef enum test_net_ws_endpoint_op {
    test_net_ws_endpoint_op_noop,
} test_net_ws_endpoint_op_t;

/*text*/
void test_net_ws_endpoint_on_msg_text(void * ctx, net_ws_endpoint_t endpoint, const char * msg) {
    uint32_t id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    check_expected(id);
    check_expected(msg);

    test_net_ws_endpoint_op_t op = mock_type(test_net_ws_endpoint_op_t);
    switch(op) {
    case test_net_ws_endpoint_op_noop:
        break;
    }
}

void test_net_ws_endpoint_install_receiver_text(net_ws_endpoint_t endpoint) {
    net_ws_endpoint_set_msg_receiver_text(endpoint, NULL, test_net_ws_endpoint_on_msg_text, NULL);
}

void test_net_ws_endpoint_expect_text_msg(net_ws_endpoint_t endpoint, const char * msg) {
    expect_value(test_net_ws_endpoint_on_msg_text, id, net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint)));
    expect_string(test_net_ws_endpoint_on_msg_text, msg, msg);
}

/*bin*/
void test_net_ws_endpoint_on_msg_bin(void * ctx, net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    uint32_t id = net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint));
    check_expected(id);
    check_expected(msg);

    test_net_ws_endpoint_op_t op = mock_type(test_net_ws_endpoint_op_t);
    switch(op) {
    case test_net_ws_endpoint_op_noop:
        break;
    }
}

void test_net_ws_endpoint_install_receiver_bin(net_ws_endpoint_t endpoint) {
    net_ws_endpoint_set_msg_receiver_bin(endpoint, NULL, test_net_ws_endpoint_on_msg_bin, NULL);
}

void test_net_ws_endpoint_expect_bin_msg(net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    expect_value(test_net_ws_endpoint_on_msg_bin, id, net_endpoint_id(net_ws_endpoint_base_endpoint(endpoint)));
    expect_memory(test_net_ws_endpoint_on_msg_bin, msg, msg, msg_len);
}

/*both*/
void test_net_ws_endpoint_install_receivers(net_ws_endpoint_t endpoint) {
    test_net_ws_endpoint_install_receiver_text(endpoint);
    test_net_ws_endpoint_install_receiver_bin(endpoint);
}
