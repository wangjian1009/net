#ifndef TESTS_UTILS_PROTOCOL_HTTP_CONN_H_INCLEDED
#define TESTS_UTILS_PROTOCOL_HTTP_CONN_H_INCLEDED
#include "net_http_test_protocol.h"

struct net_http_test_conn {
    net_http_test_protocol_t m_protocol;
    TAILQ_ENTRY(net_http_test_conn) m_next;
    net_http_endpoint_t m_endpoint;
};

net_http_test_conn_t
net_http_test_conn_create(
    net_http_test_protocol_t protocol, net_driver_t driver, const char * target);

void net_http_test_conn_free(net_http_test_conn_t conn);

#endif
