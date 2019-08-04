#ifndef NET_EBB_RESPONSE_H
#define NET_EBB_RESPONSE_H
#include "cpe/utils/utils_types.h"
#include "net_ebb_system.h"

NET_BEGIN_DECL

enum net_ebb_response_state {
    net_ebb_response_state_init,
    net_ebb_response_state_head,
    net_ebb_response_state_body,
    net_ebb_response_state_done,
};

net_ebb_response_t net_ebb_response_create(net_ebb_request_t request);
net_ebb_response_t net_ebb_response_get(net_ebb_request_t request);
void net_ebb_response_free(net_ebb_response_t response);

net_ebb_request_transfer_encoding_t net_ebb_response_transfer_encoding(net_ebb_response_t response);
net_ebb_response_state_t net_ebb_response_state(net_ebb_response_t response);
int net_ebb_response_append_code(net_ebb_response_t response, int http_code, const char * http_code_msg);
int net_ebb_response_append_head(net_ebb_response_t response, const char * name, const char * value);
int net_ebb_response_append_head_line(net_ebb_response_t response, const char * head_line);

int net_ebb_response_append_body_identity_begin(net_ebb_response_t response, uint32_t size);
int net_ebb_response_append_body_identity_data(net_ebb_response_t response, void const * data, uint32_t data_size);
int net_ebb_response_append_body_identity_from_stream(net_ebb_response_t response, read_stream_t rs);

int net_ebb_response_append_body_chunked_begin(net_ebb_response_t response);
int net_ebb_response_append_body_chunked_block(net_ebb_response_t response, void const * data, uint32_t data_size);

int net_ebb_response_append_complete(net_ebb_response_t response);

/*utils*/
int net_ebb_response_send(net_ebb_request_t request, int http_code, const char * http_code_msg);

const char * net_ebb_response_state_str(net_ebb_response_state_t state);

NET_END_DECL

#endif
