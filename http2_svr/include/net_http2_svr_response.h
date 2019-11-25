#ifndef NET_HTTP2_SVR_RESPONSE_H
#define NET_HTTP2_SVR_RESPONSE_H
#include "cpe/utils/utils_types.h"
#include "net_http2_svr_system.h"

NET_BEGIN_DECL

enum net_http2_svr_response_state {
    net_http2_svr_response_state_init,
    net_http2_svr_response_state_head,
    net_http2_svr_response_state_body,
    net_http2_svr_response_state_done,
};

net_http2_svr_response_t net_http2_svr_response_create(net_http2_svr_request_t request);
net_http2_svr_response_t net_http2_svr_response_get(net_http2_svr_request_t request);
void net_http2_svr_response_free(net_http2_svr_response_t response);

net_http2_svr_request_transfer_encoding_t net_http2_svr_response_transfer_encoding(net_http2_svr_response_t response);
net_http2_svr_response_state_t net_http2_svr_response_state(net_http2_svr_response_t response);
int net_http2_svr_response_append_code(net_http2_svr_response_t response, int http_code, const char * http_code_msg);
int net_http2_svr_response_append_head(net_http2_svr_response_t response, const char * name, const char * value);
int net_http2_svr_response_append_head_line(net_http2_svr_response_t response, const char * head_line);
int net_http2_svr_response_append_head_minetype_by_postfix(net_http2_svr_response_t response, const char * postfix, uint8_t * is_added);

int net_http2_svr_response_append_body_identity_begin(net_http2_svr_response_t response, uint32_t size);
int net_http2_svr_response_append_body_identity_data(net_http2_svr_response_t response, void const * data, uint32_t data_size);
int net_http2_svr_response_append_body_identity_from_stream(net_http2_svr_response_t response, read_stream_t rs);

int net_http2_svr_response_append_body_chunked_begin(net_http2_svr_response_t response);
int net_http2_svr_response_append_body_chunked_block(net_http2_svr_response_t response, void const * data, uint32_t data_size);

int net_http2_svr_response_append_complete(net_http2_svr_response_t response);

/*utils*/
int net_http2_svr_response_send(net_http2_svr_request_t request, int http_code, const char * http_code_msg);

const char * net_http2_svr_response_state_str(net_http2_svr_response_state_t state);

NET_END_DECL

#endif
