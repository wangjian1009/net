#ifndef NET_NGHTTP2_RESPONSE_H
#define NET_NGHTTP2_RESPONSE_H
#include "cpe/utils/utils_types.h"
#include "net_nghttp2_system.h"

NET_BEGIN_DECL

enum net_nghttp2_response_state {
    net_nghttp2_response_state_init,
    net_nghttp2_response_state_head,
    net_nghttp2_response_state_body,
    net_nghttp2_response_state_done,
};

net_nghttp2_response_t net_nghttp2_response_create(net_nghttp2_request_t request);
net_nghttp2_response_t net_nghttp2_response_get(net_nghttp2_request_t request);
void net_nghttp2_response_free(net_nghttp2_response_t response);

net_nghttp2_request_transfer_encoding_t net_nghttp2_response_transfer_encoding(net_nghttp2_response_t response);
net_nghttp2_response_state_t net_nghttp2_response_state(net_nghttp2_response_t response);
int net_nghttp2_response_append_code(net_nghttp2_response_t response, int http_code, const char * http_code_msg);
int net_nghttp2_response_append_head(net_nghttp2_response_t response, const char * name, const char * value);
int net_nghttp2_response_append_head_line(net_nghttp2_response_t response, const char * head_line);
int net_nghttp2_response_append_head_minetype_by_postfix(net_nghttp2_response_t response, const char * postfix, uint8_t * is_added);

int net_nghttp2_response_append_body_identity_begin(net_nghttp2_response_t response, uint32_t size);
int net_nghttp2_response_append_body_identity_data(net_nghttp2_response_t response, void const * data, uint32_t data_size);
int net_nghttp2_response_append_body_identity_from_stream(net_nghttp2_response_t response, read_stream_t rs);

int net_nghttp2_response_append_body_chunked_begin(net_nghttp2_response_t response);
int net_nghttp2_response_append_body_chunked_block(net_nghttp2_response_t response, void const * data, uint32_t data_size);

int net_nghttp2_response_append_complete(net_nghttp2_response_t response);

/*utils*/
int net_nghttp2_response_send(net_nghttp2_request_t request, int http_code, const char * http_code_msg);

const char * net_nghttp2_response_state_str(net_nghttp2_response_state_t state);

NET_END_DECL

#endif
