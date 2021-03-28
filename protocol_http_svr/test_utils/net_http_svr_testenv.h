#ifndef TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#define TESTS_NET_HTTP_SVR_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "net_system.h"
#include "net_http_svr_system.h"

typedef struct net_http_svr_testenv * net_http_svr_testenv_t;

struct net_http_svr_testenv {
    net_schedule_t m_schedule;
    test_net_driver_t m_driver;
    net_http_svr_protocol_t m_protocol;
    net_acceptor_t m_acceptor;
};

net_http_svr_testenv_t
net_http_svr_testenv_create(net_schedule_t schedule, test_net_driver_t driver);

void net_http_svr_testenv_free(net_http_svr_testenv_t env);

void net_http_svr_testenv_listen(net_http_svr_testenv_t env, const char * address);

#endif
