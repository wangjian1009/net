#ifndef NET_NGHTTP2_REQUEST_H
#define NET_NGHTTP2_REQUEST_H
#include "cpe/utils/utils_types.h"
#include "net_nghttp2_system.h"

NET_BEGIN_DECL

enum net_nghttp2_request_method {
    net_nghttp2_request_method_unknown,
    net_nghttp2_request_method_copy,
    net_nghttp2_request_method_delete,
    net_nghttp2_request_method_get,
    net_nghttp2_request_method_head,
    net_nghttp2_request_method_lock,
    net_nghttp2_request_method_mkcol,
    net_nghttp2_request_method_move,
    net_nghttp2_request_method_options,
    net_nghttp2_request_method_post,
    net_nghttp2_request_method_propfind,
    net_nghttp2_request_method_proppatch,
    net_nghttp2_request_method_put,
    net_nghttp2_request_method_trace,
    net_nghttp2_request_method_unlock,
};
    
enum net_nghttp2_request_transfer_encoding {
    net_nghttp2_request_transfer_encoding_unknown,
    net_nghttp2_request_transfer_encoding_identity,
    net_nghttp2_request_transfer_encoding_chunked,
};

enum net_nghttp2_request_state {
    net_nghttp2_request_state_reading,
    net_nghttp2_request_state_processing,
    net_nghttp2_request_state_complete,
};

void net_nghttp2_request_free(net_nghttp2_request_t request);

uint32_t net_nghttp2_request_id(net_nghttp2_request_t request);
net_nghttp2_request_method_t net_nghttp2_request_method(net_nghttp2_request_t request);
net_nghttp2_request_transfer_encoding_t net_nghttp2_request_transfer_encoding(net_nghttp2_request_t request);
uint8_t net_nghttp2_request_expect_continue(net_nghttp2_request_t request);
uint16_t net_nghttp2_request_version_major(net_nghttp2_request_t request);
uint16_t net_nghttp2_request_version_minor(net_nghttp2_request_t request);

const char * net_nghttp2_request_full_path(net_nghttp2_request_t request);
const char * net_nghttp2_request_relative_path(net_nghttp2_request_t request);

net_nghttp2_request_state_t net_nghttp2_request_state(net_nghttp2_request_t request);
void net_nghttp2_request_schedule_close_connection(net_nghttp2_request_t request);

void net_nghttp2_request_print(write_stream_t ws, net_nghttp2_request_t request);
const char * net_nghttp2_request_dump(mem_buffer_t buffer, net_nghttp2_request_t request);

void * net_nghttp2_request_data(net_nghttp2_request_t request);
net_nghttp2_request_t net_nghttp2_request_from_data(void * data);

/**/
const char * net_nghttp2_request_method_str(net_nghttp2_request_method_t method);
const char * net_nghttp2_request_state_str(net_nghttp2_request_state_t state);
const char * net_nghttp2_request_transfer_encoding_str(net_nghttp2_request_transfer_encoding_t transfer_encoding);

NET_END_DECL

#endif
