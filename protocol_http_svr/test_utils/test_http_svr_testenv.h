#ifndef TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#define TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "net_http_test_protocol.h"
#include "net_system.h"
#include "net_http_svr_system.h"

typedef struct test_http_svr_testenv * test_http_svr_testenv_t;
typedef struct test_http_svr_mock_svr * test_http_svr_mock_svr_t;
typedef TAILQ_HEAD(test_http_svr_mock_svr_list, test_http_svr_mock_svr) test_http_svr_mock_svr_list_t;

struct test_http_svr_testenv {
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_driver;
    net_http_test_protocol_t m_cli_protocol;
    test_http_svr_mock_svr_list_t m_svrs;
};

test_http_svr_testenv_t
test_http_svr_testenv_create(
    net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em);

void test_http_svr_testenv_free(test_http_svr_testenv_t env);

/*创建客户端请求 */
net_http_req_t
test_http_svr_testenv_create_req(
    test_http_svr_testenv_t env, const char * svr_url, net_http_req_method_t method, const char * url);

net_http_test_response_t
test_http_svr_testenv_req_commit(test_http_svr_testenv_t env, net_http_req_t req);

net_endpoint_t
test_http_svr_testenv_req_svr_endpoint(test_http_svr_testenv_t env, net_http_req_t req);

/*设置服务端数据 */
void test_http_svr_testenv_create_mock_svr(
    test_http_svr_testenv_t env, const char * name, const char * url);

void test_http_svr_testenv_expect_response(
    test_http_svr_testenv_t env, const char * url, const char * path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms);

void test_http_svr_testenv_expect_response_close(
    test_http_svr_testenv_t env, const char * url, const char * path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms);


#endif
