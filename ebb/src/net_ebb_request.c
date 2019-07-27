#include "net_ebb_request_i.h"

void net_ebb_request_init(net_ebb_request_t request) {
    request->expect_continue = 0;
    request->body_read = 0;
    request->content_length = 0;
    request->version_major = 0;
    request->version_minor = 0;
    request->number_of_headers = 0;
    request->transfer_encoding = EBB_IDENTITY;
    request->keep_alive = -1;

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

uint8_t net_ebb_request_should_keep_alive(net_ebb_request_t request) {
    if (request->keep_alive == -1) {
        if (request->version_major == 1) {
            return (request->version_minor != 0);
        }
        else if (request->version_major == 0) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        return request->keep_alive;
    }
}
