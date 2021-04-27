#ifndef TESTS_NET_HTTP_SVR_MOCK_REQUEST_H_INCLEDED
#define TESTS_NET_HTTP_SVR_MOCK_REQUEST_H_INCLEDED
#include "test_net_driver.h"
#include "net_http_svr_system.h"

typedef enum net_http_svr_mock_request_policy_type net_http_svr_mock_request_policy_type_t;
typedef struct net_http_svr_mock_request_policy * net_http_svr_mock_request_policy_t;

enum net_http_svr_mock_request_policy_type {
    net_http_svr_mock_request_type_response,
    net_http_svr_mock_request_type_noop,
};

struct net_http_svr_mock_request_policy {
    net_http_svr_mock_request_policy_type_t m_type;
    union {
        struct {
            uint32_t m_delay_ms;
            uint8_t m_keep_alive;
            int m_code;
            char * m_code_msg;
            char * m_body;
        } m_response;
    };
};

void net_http_svr_request_mock_apply(
    test_net_driver_t driver, net_http_svr_request_t request, net_http_svr_mock_request_policy_t policy);

net_http_svr_mock_request_policy_t
net_http_svr_request_mock_expect_response(
    test_net_driver_t driver, int code, const char * code_msg, const char * response, uint32_t delay_ms);

net_http_svr_mock_request_policy_t
net_http_svr_request_mock_expect_response_close(
    test_net_driver_t driver, int code, const char * code_msg, const char * response, uint32_t delay_ms);

#endif
