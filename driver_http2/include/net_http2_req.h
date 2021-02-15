#ifndef NET_HTTP2_REQ_H_INCLEDED
#define NET_HTTP2_REQ_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_req_state {
    net_http2_req_state_init,
    net_http2_req_state_connecting,
    net_http2_req_state_established,
    net_http2_req_state_read_closed,
    net_http2_req_state_write_closed,
    net_http2_req_state_error,
    net_http2_req_state_done,
};

enum net_http2_req_method {
    net_http2_req_method_get,
    net_http2_req_method_post,
    net_http2_req_method_head,
};

enum net_http2_res_state {
    net_http2_res_state_reading_head,
    net_http2_res_state_reading_body,
    net_http2_res_state_completed,
};

net_http2_req_t net_http2_req_create(net_http2_endpoint_t http_ep, net_http2_req_method_t method, const char * url);
void net_http2_req_free(net_http2_req_t req);

net_http2_req_t net_http2_req_find(net_http2_endpoint_t http_ep, uint16_t req_id);

void net_http2_req_cancel_and_free(net_http2_req_t req);
void net_http2_req_cancel_and_free_by_id(net_http2_endpoint_t http_ep, uint16_t req_id);

uint16_t net_http2_req_id(net_http2_req_t req);

/*req*/
net_http2_endpoint_t net_http2_req_endpoint(net_http2_req_t req);
net_http2_stream_t net_http2_req_stream(net_http2_req_t req);
net_http2_req_method_t net_http2_req_method(net_http2_req_t req);
net_http2_req_state_t net_http2_req_state(net_http2_req_t req);
int net_http2_req_add_req_head(net_http2_req_t http_req, const char * attr_name, const char * attr_value);
int net_http2_req_start(net_http2_req_t http_req);

const char * net_http2_req_state_str(net_http2_req_state_t req_state);

/*response*/
typedef enum net_http2_res_op_result {
    net_http2_res_op_success,
    net_http2_res_op_ignore,
    net_http2_res_op_error_and_close,
} net_http2_res_op_result_t;

typedef enum net_http2_res_result {
    net_http2_res_complete,
    net_http2_res_timeout,
    net_http2_res_canceled,
    net_http2_res_disconnected,
} net_http2_res_result_t;

typedef net_http2_res_op_result_t (*net_http2_req_on_res_head_fun_t)(void * ctx, net_http2_req_t req);
typedef net_http2_res_op_result_t (*net_http2_req_on_res_data_fun_t)(void * ctx, net_http2_req_t req, void * data, size_t data_size);
typedef net_http2_res_op_result_t (*net_http2_req_on_res_complete_fun_t)(void * ctx, net_http2_req_t req, net_http2_res_result_t result);

int net_http2_req_set_reader(
    net_http2_req_t req,
    void * ctx,
    net_http2_req_on_res_head_fun_t on_head,
    net_http2_req_on_res_data_fun_t on_data,
    net_http2_req_on_res_complete_fun_t on_complete);

void net_http2_req_clear_reader(net_http2_req_t req);

uint16_t net_http2_req_res_code(net_http2_req_t req);

const char *  net_http2_req_method_str( net_http2_req_method_t method);
const char * net_http2_res_state_str(net_http2_res_state_t res_state);
const char * net_http2_res_result_str(net_http2_res_result_t res_result);

NET_END_DECL

#endif
