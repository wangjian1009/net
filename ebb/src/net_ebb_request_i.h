#ifndef NET_EBB_REQUEST_H_I_INCLEDED
#define NET_EBB_REQUEST_H_I_INCLEDED
#include "net_ebb_request.h"
#include "net_ebb_connection_i.h"

typedef void (*net_ebb_header_cb)(net_ebb_request_t, const char* at, size_t length, int header_index);
typedef void (*net_ebb_element_cb)(net_ebb_request_t, const char* at, size_t length);

struct net_ebb_request {
    net_ebb_connection_t m_connection;
    TAILQ_ENTRY(net_ebb_request) m_next;
    
    net_ebb_request_method_t m_method;
    net_ebb_request_transfer_encoding_t m_transfer_encoding; /* ro */
    uint8_t m_expect_continue; /* ro */
    uint16_t m_version_major; /* ro */
    uint16_t m_version_minor; /* ro */
    uint16_t m_number_of_headers; /* ro */
    int8_t m_keep_alive; /* private - use net_ebb_request_should_keep_alive */
    size_t m_content_length; /* ro - 0 if unknown */
    size_t m_body_read; /* ro */

    /* Public  - ordered list of callbacks */
    net_ebb_element_cb on_path;
    net_ebb_element_cb on_query_string;
    net_ebb_element_cb on_uri;
    net_ebb_element_cb on_fragment;
    net_ebb_header_cb on_header_field;
    net_ebb_header_cb on_header_value;
    void (*on_headers_complete)(net_ebb_request_t);
    net_ebb_element_cb on_body;
    void (*on_complete)(net_ebb_request_t);
    void* data;
};

net_ebb_request_t net_ebb_request_create(net_ebb_connection_t connection);
void net_ebb_request_free(net_ebb_request_t request);
void net_ebb_request_real_free(net_ebb_request_t request);

uint8_t net_ebb_request_should_keep_alive(net_ebb_request_t request);

#define net_ebb_request_has_body(request)                               \
    (request->transfer_encoding == EBB_CHUNKED || request->content_length > 0)

#endif
