#include "cmocka_all.h"
#include "cpe/pal/pal_string.h"
#include "net_ws_tests.h"
#include "net_ws_testenv.h"

static int setup(void **state) {
    net_ws_testenv_t env = net_ws_testenv_create();
    *state = env;
    return 0;
}

static int teardown(void **state) {
    net_ws_testenv_t env = *state;
    net_ws_testenv_free(env);
    return 0;
}

static void net_ws_svr_basic(void **state) {
    net_ws_testenv_t env = *state;

    net_ws_endpoint_t endpoint = net_ws_testenv_svr_ep_create(env);
    assert_true(endpoint);

    net_endpoint_t base_endpoint = net_ws_endpoint_base_endpoint(endpoint);
    
    net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
    
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(endpoint)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_init));

    /*尝试一个字节一个字节输入，确保状态处理正确 */
    const char * handshake_request =
        "GET /a/b HTTP/1.1\r\n"
        "Host: 1.1.2.3:5678\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: QMBTF6u1wEahS5R5TrnVDg==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    uint32_t total_len = strlen(handshake_request);
    uint32_t pos = 0;
    for(; pos < total_len - 1; pos++) {
        net_endpoint_buf_append(base_endpoint, net_ep_buf_read, handshake_request + pos, 1);
        assert_string_equal(
            net_ws_endpoint_state_str(net_ws_endpoint_state(endpoint)),
            net_ws_endpoint_state_str(net_ws_endpoint_state_handshake));
    }

    test_net_endpoint_expect_write_keep(base_endpoint, net_ep_buf_user1);
    
    net_endpoint_buf_append(base_endpoint, net_ep_buf_read, handshake_request + pos, 1);
    assert_string_equal(
        net_ws_endpoint_state_str(net_ws_endpoint_state(endpoint)),
        net_ws_endpoint_state_str(net_ws_endpoint_state_streaming));

    uint32_t response_sz = net_endpoint_buf_size(base_endpoint, net_ep_buf_user1);
    char zero = 0;
    net_endpoint_buf_append(base_endpoint, net_ep_buf_user1, &zero, 1);

    char * response = NULL;
    assert_true(net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_user1, response_sz + 1, (void**)&response) == 0);
    assert_true(response);

    assert_string_equal(
        response,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: 6s4Fm5KdL9gsEmd12ZQJl++ux/c=\r\n"
        "\r\n");
}

CPE_BEGIN_TEST_SUIT(net_ws_svr_basic_tests)
    cmocka_unit_test_setup_teardown(net_ws_svr_basic, setup, teardown),
CPE_END_TEST_SUIT(net_ws_svr_basic_tests)
