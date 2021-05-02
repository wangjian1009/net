#ifndef TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#define TESTS_NET_HTTP_SVR_MOCK_SVR_H_INCLEDED
#include "cpe/utils/url.h"
#include "net_http_svr_testenv.h"

struct net_http_svr_mock_svr {
    net_http_svr_testenv_t m_env;
    TAILQ_ENTRY(net_http_svr_mock_svr) m_next;
    char * m_url;
    net_http_svr_protocol_t m_http_protocol;
    net_http_svr_processor_t m_http_processor;
    net_driver_t m_ssl_driver;
    net_acceptor_t m_acceptor;
};

net_http_svr_mock_svr_t
net_http_svr_mock_svr_create(net_http_svr_testenv_t env, const char * name, const char * url);

void net_http_svr_mock_svr_free(net_http_svr_mock_svr_t svr);

net_http_svr_mock_svr_t
net_http_svr_mock_svr_find(net_http_svr_testenv_t env, const char * url);

#endif
