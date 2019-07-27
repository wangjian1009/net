#ifndef NET_EBB_REQUEST_H_I_INCLEDED
#define NET_EBB_REQUEST_H_I_INCLEDED
#include "net_ebb_request.h"
#include "net_ebb_connection_i.h"

struct net_ebb_request {
    int method;
    int transfer_encoding; /* ro */
    int expect_continue; /* ro */
    unsigned int version_major; /* ro */
    unsigned int version_minor; /* ro */
    int number_of_headers; /* ro */
    int keep_alive; /* private - use net_ebb_request_should_keep_alive */
    size_t content_length; /* ro - 0 if unknown */
    size_t body_read; /* ro */

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

void net_ebb_request_init(net_ebb_request_t);
uint8_t net_ebb_request_should_keep_alive(net_ebb_request_t request);

#define net_ebb_request_has_body(request)                               \
    (request->transfer_encoding == EBB_CHUNKED || request->content_length > 0)

#endif
