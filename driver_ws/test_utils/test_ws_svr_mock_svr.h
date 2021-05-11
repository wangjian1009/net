#ifndef TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#define TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#include "cpe/utils/url.h"
#include "test_ws_svr_testenv.h"
#include "test_ws_endpoint_action.h"

typedef struct test_ws_svr_mock_svr_action * test_ws_svr_mock_svr_action_t;
typedef TAILQ_HEAD(test_ws_svr_mock_svr_action_list, test_ws_svr_mock_svr_action) test_ws_svr_mock_svr_action_list_t;

struct test_ws_svr_mock_svr {
    test_ws_svr_testenv_t m_env;
    TAILQ_ENTRY(test_ws_svr_mock_svr) m_next;
    char * m_url;
    net_ws_protocol_t m_ws_protocol;
    net_driver_t m_ssl_driver;
    net_acceptor_t m_acceptor;
    test_ws_svr_mock_svr_action_list_t m_actions;
};

test_ws_svr_mock_svr_t
test_ws_svr_mock_svr_create(test_ws_svr_testenv_t env, const char * name, const char * url);

void test_ws_svr_mock_svr_free(test_ws_svr_mock_svr_t svr);

test_ws_svr_mock_svr_t
test_ws_svr_mock_svr_find(test_ws_svr_testenv_t env, const char * url);

void test_ws_svr_mock_svr_on_connected(
    test_ws_svr_mock_svr_t svr, test_net_ws_endpoint_action_t action, int64_t delay_ms);
    
#endif
