#ifndef TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#define TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#include "cpe/utils/url.h"
#include "test_http_svr_testenv.h"

struct test_http_svr_mock_svr {
    test_http_svr_testenv_t m_env;
    TAILQ_ENTRY(test_http_svr_mock_svr) m_next;
    char * m_url;
    net_http_svr_protocol_t m_http_protocol;
    net_http_svr_processor_t m_http_processor;
    net_driver_t m_ssl_driver;
    net_acceptor_t m_acceptor;
};

test_http_svr_mock_svr_t
test_http_svr_mock_svr_create(test_http_svr_testenv_t env, const char * name, const char * url);

void test_http_svr_mock_svr_free(test_http_svr_mock_svr_t svr);

test_http_svr_mock_svr_t
test_http_svr_mock_svr_find(test_http_svr_testenv_t env, const char * url);

#endif
