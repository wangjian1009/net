#include "net_ebb_request_i.h"

void net_ebb_request_init(net_ebb_request_t request) {
    request->m_expect_continue = 0;
    request->m_body_read = 0;
    request->m_content_length = 0;
    request->m_version_major = 0;
    request->m_version_minor = 0;
    request->m_number_of_headers = 0;
    request->m_transfer_encoding = net_ebb_request_transfer_encoding_identity;
    request->m_keep_alive = -1;

    request->on_complete = NULL;
    request->on_headers_complete = NULL;
    request->on_body = NULL;
    request->on_header_field = NULL;
    request->on_header_value = NULL;
    request->on_uri = NULL;
    request->on_fragment = NULL;
    request->on_path = NULL;
    request->on_query_string = NULL;
}

uint32_t net_ebb_request_method(net_ebb_request_t request) {
    return request->m_method;
}

net_ebb_request_transfer_encoding_t net_ebb_request_transfer_encoding(net_ebb_request_t request) {
    return request->m_transfer_encoding;
}

uint8_t net_ebb_request_expect_continue(net_ebb_request_t request) {
    return request->m_expect_continue;
}

uint16_t net_ebb_request_version_major(net_ebb_request_t request) {
    return request->m_version_major;
}

uint16_t net_ebb_request_version_minor(net_ebb_request_t request) {
    return request->m_version_minor;
}

uint8_t net_ebb_request_should_keep_alive(net_ebb_request_t request) {
    if (request->m_keep_alive == -1) {
        if (request->m_version_major == 1) {
            return (request->m_version_minor != 0);
        }
        else if (request->m_version_major == 0) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        return request->m_keep_alive;
    }
}
