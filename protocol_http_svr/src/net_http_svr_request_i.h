#ifndef NET_HTTP_SVR_REQUEST_H_I_INCLEDED
#define NET_HTTP_SVR_REQUEST_H_I_INCLEDED
#include "net_http_svr_request.h"
#include "net_http_svr_endpoint_i.h"

typedef void (*net_http_svr_header_cb)(net_http_svr_request_t, const char* at, size_t length, int header_index);
typedef void (*net_http_svr_element_cb)(net_http_svr_request_t, const char* at, size_t length);

struct net_http_svr_request {
    net_endpoint_t m_base_endpoint;
    TAILQ_ENTRY(net_http_svr_request) m_next_for_connection;
    net_http_svr_processor_t m_processor;
    TAILQ_ENTRY(net_http_svr_request) m_next_for_processor;
    uint32_t m_request_id;
    struct cpe_hash_entry m_hh_for_protocol;
    void * m_processor_request;
    net_http_svr_request_method_t m_method;
    net_http_svr_request_transfer_encoding_t m_transfer_encoding;
    uint8_t m_expect_continue;
    uint16_t m_version_major;
    uint16_t m_version_minor;
    uint16_t m_number_of_headers;
    int8_t m_keep_alive;
    char * m_host;
    char * m_path;
    const char * m_path_to_processor;
    net_http_svr_request_header_list_t m_headers;
    net_http_svr_request_state_t m_state;
    size_t m_content_length;
    size_t m_body_read;
    net_http_svr_response_t m_response;
};

net_http_svr_request_t net_http_svr_request_create(net_endpoint_t base_endpoint);
void net_http_svr_request_real_free(net_http_svr_request_t request);

uint8_t net_http_svr_request_should_keep_alive(net_http_svr_request_t request);

void net_http_svr_request_on_path(net_http_svr_request_t request, const char* at, size_t length);
void net_http_svr_request_on_query_string(net_http_svr_request_t request, const char* at, size_t length);
void net_http_svr_request_on_uri(net_http_svr_request_t request, const char* at, size_t length);
void net_http_svr_request_on_fragment(net_http_svr_request_t request, const char* at, size_t length);
void net_http_svr_request_on_header_field(net_http_svr_request_t request, const char* at, size_t length, int header_index);
void net_http_svr_request_on_header_value(net_http_svr_request_t request, const char* at, size_t length, int header_index);
void net_http_svr_request_on_body(net_http_svr_request_t request, const char* at, size_t length);
void net_http_svr_request_on_head_complete(net_http_svr_request_t request);
void net_http_svr_request_on_complete(net_http_svr_request_t request);

void net_http_svr_request_set_processor(net_http_svr_request_t request, net_http_svr_mount_point_t mp);
void net_http_svr_request_set_state(net_http_svr_request_t request, net_http_svr_request_state_t state);

uint32_t net_http_svr_request_hash(net_http_svr_request_t o);
int net_http_svr_request_eq(net_http_svr_request_t l, net_http_svr_request_t r);

#define net_http_svr_request_has_body(request)                               \
    (request->transfer_encoding == HTTP_SVR_CHUNKED || request->content_length > 0)

#endif
