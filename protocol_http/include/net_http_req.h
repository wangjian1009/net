#ifndef NET_HTTP_REQ_H_INCLEDED
#define NET_HTTP_REQ_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_req_t net_http_req_create(net_http_endpoint_t http_ep, net_http_req_method_t method, const char * url);
void net_http_req_free(net_http_req_t req);

uint16_t net_http_req_id(net_http_req_t req);
net_http_endpoint_t net_http_req_ep(net_http_req_t req);

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
    net_http_res_success,
    net_http_res_ignore,
    net_http_res_error_and_reconnect,
} net_http_res_op_result_t;

typedef net_http_res_op_result_t (*net_http_req_on_res_begin_fun_t)(void * ctx, net_http_req_t req, uint16_t code, const char * msg);
typedef net_http_res_op_result_t (*net_http_req_on_res_head_fun_t)(void * ctx, net_http_req_t req, const char * name, const char * value);
typedef net_http_res_op_result_t (*net_http_req_on_res_body_fun_t)(void * ctx, net_http_req_t req);
typedef net_http_res_op_result_t (*net_http_req_on_res_complete_fun_t)(void * ctx, net_http_req_t req);

int net_http_req_set_reader(
    net_http_req_t req,
    void * ctx,
    net_http_req_on_res_begin_fun_t on_begin,
    net_http_req_on_res_head_fun_t on_head,
    net_http_req_on_res_body_fun_t on_body,
    net_http_req_on_res_complete_fun_t on_complete);

const char * net_http_res_state_str(net_http_res_state_t res_state);

NET_END_DECL

#endif