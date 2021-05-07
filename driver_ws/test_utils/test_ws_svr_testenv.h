#ifndef TESTS_NET_WS_SVR_TESTENV_H_INCLEDED
#define TESTS_NET_WS_SVR_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "net_system.h"
#include "net_ws_types.h"

typedef struct test_ws_svr_testenv * test_ws_svr_testenv_t;
typedef struct test_ws_svr_mock_svr * test_ws_svr_mock_svr_t;
typedef TAILQ_HEAD(test_ws_svr_mock_svr_list, test_ws_svr_mock_svr) test_ws_svr_mock_svr_list_t;

struct test_ws_svr_testenv {
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_driver;
    test_ws_svr_mock_svr_list_t m_svrs;
};

test_ws_svr_testenv_t
test_ws_svr_testenv_create(
    net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em);

void test_ws_svr_testenv_free(test_ws_svr_testenv_t env);

net_ws_endpoint_t
test_ws_svr_testenv_svr_endpoint(test_ws_svr_testenv_t env, net_ws_endpoint_t cli_endpoint);

/*设置服务端数据 */
void test_ws_svr_testenv_create_mock_svr(
    test_ws_svr_testenv_t env, const char * name, const char * url);

void test_ws_svr_testenv_send_bin_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, void const * msg, uint32_t msg_len, int64_t delay_ms);

void test_ws_svr_testenv_send_text_msg(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, const char * msg, int64_t delay_ms);

void test_ws_svr_testenv_close(
    test_ws_svr_testenv_t env, net_ws_endpoint_t endpoint, uint16_t status_code, const char * msg, int64_t delay_ms);

#endif
