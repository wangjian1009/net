#ifndef NET_HTTP_REQ_H_INCLEDED
#define NET_HTTP_REQ_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_req_t net_http_req_create(net_http_endpoint_t http_ep, net_http_req_method_t method, const char * url);
void net_http_req_free(net_http_req_t req);

uint16_t net_http_req_id(net_http_req_t req);
net_http_endpoint_t net_http_req_ep(net_http_req_t req);

net_http_req_t net_http_req_find(net_http_endpoint_t http_ep, uint16_t req_id);

int net_http_req_start_timer(net_http_req_t req, uint32_t timeout_ms);

void net_http_req_cancel_and_free(net_http_req_t req);
void net_http_req_cancel_and_free_by_id(net_http_endpoint_t http_ep, uint16_t req_id);
    
/*write*/
net_http_req_state_t net_http_req_state(net_http_req_t req);
int net_http_req_write_head_host(net_http_req_t http_req);
int net_http_req_write_head_pair(net_http_req_t http_req, const char * attr_name, const char * attr_value);
int net_http_req_write_body_full(net_http_req_t http_req, void const * data, size_t data_sz);
int net_http_req_write_commit(net_http_req_t http_req);

const char * net_http_req_state_str(net_http_req_state_t req_state);

/*read*/
net_http_res_state_t net_http_req_res_state(net_http_req_t req);

typedef enum net_http_res_op_result {
    net_http_res_op_success,
    net_http_res_op_ignore,
    net_http_res_op_error_and_reconnect,
} net_http_res_op_result_t;

typedef enum net_http_res_result {
    net_http_res_complete,
    net_http_res_timeout,
    net_http_res_canceled,
    net_http_res_disconnected,
} net_http_res_result_t;

typedef net_http_res_op_result_t (*net_http_req_on_res_begin_fun_t)(void * ctx, net_http_req_t req, uint16_t code, const char * msg);
typedef net_http_res_op_result_t (*net_http_req_on_res_head_fun_t)(void * ctx, net_http_req_t req, const char * name, const char * value);
typedef net_http_res_op_result_t (*net_http_req_on_res_body_fun_t)(void * ctx, net_http_req_t req, void * data, size_t data_size);
typedef net_http_res_op_result_t (*net_http_req_on_res_complete_fun_t)(void * ctx, net_http_req_t req, net_http_res_result_t result);

int net_http_req_set_reader(
    net_http_req_t req,
    void * ctx,
    net_http_req_on_res_begin_fun_t on_begin,
    net_http_req_on_res_head_fun_t on_head,
    net_http_req_on_res_body_fun_t on_body,
    net_http_req_on_res_complete_fun_t on_complete);

uint16_t net_http_req_res_code(net_http_req_t req);
const char * net_http_req_res_message(net_http_req_t req);
uint32_t net_http_req_res_length(net_http_req_t req);

const char * net_http_res_state_str(net_http_res_state_t res_state);

const char * net_http_res_result_str(net_http_res_result_t res_result);

NET_END_DECL

#endif
