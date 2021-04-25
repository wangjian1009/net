#ifndef TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#define TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "net_http_test_protocol.h"
#include "net_system.h"
#include "net_http_svr_system.h"

typedef struct net_http_svr_testenv * net_http_svr_testenv_t;

struct net_http_svr_testenv {
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_driver;
    net_http_test_protocol_t m_cli_protocol;
    net_http_svr_protocol_t m_protocol;
    net_acceptor_t m_acceptor;
};

net_http_svr_testenv_t
net_http_svr_testenv_create(
    net_schedule_t schedule, test_net_driver_t driver, error_monitor_t em);

void net_http_svr_testenv_free(net_http_svr_testenv_t env);

void net_http_svr_testenv_listen(net_http_svr_testenv_t env, const char * address);

net_http_req_t
net_http_svr_testenv_create_req(
    net_http_svr_testenv_t env, net_http_req_method_t method, const char * url);

net_http_test_response_t
net_http_svr_testenv_req_commit(net_http_svr_testenv_t env, net_http_req_t req);

net_endpoint_t
net_http_svr_testenv_req_svr_endpoint(net_http_svr_testenv_t env, net_http_req_t req);

#endif
