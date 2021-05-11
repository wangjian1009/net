#ifndef TESTS_NET_HTTP_SVR_MOCK_SVR_ACTION_H_INCLEDED
#define TESTS_NET_HTTP_SVR_MOCK_SVR_ACTION_H_INCLEDED
#include "test_ws_svr_mock_svr.h"

enum test_ws_svr_mock_svr_action_trigger {
    test_ws_svr_mock_svr_action_connected,
    test_ws_svr_mock_svr_action_bin_msg,
    test_ws_svr_mock_svr_action_text_msg,
    test_ws_svr_mock_svr_action_close,
};

struct test_ws_svr_mock_svr_action {
    test_ws_svr_mock_svr_t m_svr;
    TAILQ_ENTRY(test_ws_svr_mock_svr_action) m_next;
    uint32_t m_effect_count;
    enum test_ws_svr_mock_svr_action_trigger m_trigger;
    int64_t m_delay_ms;
    struct test_net_ws_endpoint_action m_action;
};

test_ws_svr_mock_svr_action_t
test_ws_svr_mock_svr_action_create(
    test_ws_svr_mock_svr_t svr,
    uint32_t effect_count, enum test_ws_svr_mock_svr_action_trigger trigger,
    test_net_ws_endpoint_action_t action, int64_t delay_ms);

void test_ws_svr_mock_svr_action_free(test_ws_svr_mock_svr_action_t svr_action);

void test_ws_svr_mock_svr_apply_acctions(
    test_ws_svr_mock_svr_t svr, net_ws_endpoint_t endpint, enum test_ws_svr_mock_svr_action_trigger trigger);

#endif
