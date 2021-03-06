#ifndef NET_HTTP_SVR_REQUEST_HEADER_H_I_INCLEDED
#define NET_HTTP_SVR_REQUEST_HEADER_H_I_INCLEDED
#include "net_http_svr_request_header.h"
#include "net_http_svr_request_i.h"

struct net_http_svr_request_header {
    net_http_svr_request_t m_request;
    TAILQ_ENTRY(net_http_svr_request_header) m_next;
    uint16_t m_index;
    char * m_name;
    char * m_value;
    char m_buf[128];
};

net_http_svr_request_header_t net_http_svr_request_header_create(
    net_http_svr_request_t request, uint16_t index, const char * name, size_t name_len);
void net_http_svr_request_header_free(net_http_svr_request_header_t header);
void net_http_svr_request_header_real_free(net_http_svr_request_header_t header);

net_http_svr_request_header_t net_http_svr_request_header_find_by_index(net_http_svr_request_t request, uint16_t index);

int net_http_svr_request_header_set_value(net_http_svr_request_header_t header, const char * value, size_t value_len);

#endif
