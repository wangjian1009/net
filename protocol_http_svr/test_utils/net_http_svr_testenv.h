#ifndef TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#define TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "net_http_test_protocol.h"
#include "net_system.h"
#include "net_http_svr_system.h"

typedef struct net_http_svr_testenv * net_http_svr_testenv_t;
typedef struct net_http_svr_mock_svr * net_http_svr_mock_svr_t;
typedef TAILQ_HEAD(net_http_svr_mock_svr_list, net_http_svr_mock_svr) net_http_svr_mock_svr_list_t;

struct net_http_svr_testenv {
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_driver;
    net_http_test_protocol_t m_cli_protocol;
    net_http_svr_mock_svr_list_t m_svrs;
};

net_http_svr_testenv_t
net_http_svr_testenv_create(
    net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em);

void net_http_svr_testenv_free(net_http_svr_testenv_t env);

/*创建客户端请求 */
net_http_req_t
net_http_svr_testenv_create_req(
    net_http_svr_testenv_t env, const char * svr_url, net_http_req_method_t method, const char * url);

net_http_test_response_t
net_http_svr_testenv_req_commit(net_http_svr_testenv_t env, net_http_req_t req);

net_endpoint_t
net_http_svr_testenv_req_svr_endpoint(net_http_svr_testenv_t env, net_http_req_t req);

/*设置服务端数据 */
void net_http_svr_testenv_create_mock_svr(
    net_http_svr_testenv_t env, const char * name, const char * url);

void net_http_svr_testenv_expect_response(
    net_http_svr_testenv_t env, const char * url, const char * path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms);

void net_http_svr_testenv_expect_response_close(
    net_http_svr_testenv_t env, const char * url, const char * path,
    int code, const char * code_msg, const char * response,
    uint32_t delay_ms);


#endif
