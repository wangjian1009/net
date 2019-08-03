#ifndef NET_EBB_REQUEST_H
#define NET_EBB_REQUEST_H
#include "cpe/utils/utils_types.h"
#include "net_ebb_system.h"

NET_BEGIN_DECL

enum net_ebb_request_method {
    net_ebb_request_method_unknown,
    net_ebb_request_method_copy,
    net_ebb_request_method_delete,
    net_ebb_request_method_get,
    net_ebb_request_method_head,
    net_ebb_request_method_lock,
    net_ebb_request_method_mkcol,
    net_ebb_request_method_move,
    net_ebb_request_method_options,
    net_ebb_request_method_post,
    net_ebb_request_method_propfind,
    net_ebb_request_method_proppatch,
    net_ebb_request_method_put,
    net_ebb_request_method_trace,
    net_ebb_request_method_unlock,
};
    
enum net_ebb_request_transfer_encoding {
    net_ebb_request_transfer_encoding_unknown,
    net_ebb_request_transfer_encoding_identity,
    net_ebb_request_transfer_encoding_chunked,
};

enum net_ebb_request_state {
    net_ebb_request_state_processing,
    net_ebb_request_state_complete,
};

void net_ebb_request_free(net_ebb_request_t request);

net_ebb_request_method_t net_ebb_request_method(net_ebb_request_t request);
net_ebb_request_transfer_encoding_t net_ebb_request_transfer_encoding(net_ebb_request_t request);
uint8_t net_ebb_request_expect_continue(net_ebb_request_t request);
uint16_t net_ebb_request_version_major(net_ebb_request_t request);
uint16_t net_ebb_request_version_minor(net_ebb_request_t request);

const char * net_ebb_request_full_path(net_ebb_request_t request);
const char * net_ebb_request_relative_path(net_ebb_request_t request);

net_ebb_request_state_t net_ebb_request_state(net_ebb_request_t request);
void net_ebb_request_schedule_close_connection(net_ebb_request_t request);

void net_ebb_request_print(write_stream_t ws, net_ebb_request_t request);
const char * net_ebb_request_dump(mem_buffer_t buffer, net_ebb_request_t request);

void * net_ebb_request_data(net_ebb_request_t request);
net_ebb_request_t net_ebb_request_from_data(void * data);

/**/
const char * net_ebb_request_method_str(net_ebb_request_method_t method);
const char * net_ebb_request_state_str(net_ebb_request_state_t state);

NET_END_DECL

#endif
