#ifndef NET_HTTP_REQ_H_INCLEDED
#define NET_HTTP_REQ_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_req_t net_http_req_create(net_http_endpoint_t http_ep, net_http_req_method_t method, const char * url);
void net_http_req_free(net_http_req_t req);

uint16_t net_http_req_id(net_http_req_t req);
net_http_req_state_t net_http_req_state(net_http_req_t req);

net_http_endpoint_t net_http_req_ep(net_http_req_t req);

typedef enum net_http_res_op_result {
    net_http_res_success,
    net_http_res_ignore,
    net_http_res_error_and_reconnect,
} net_http_res_op_result_t;

typedef net_http_res_op_result_t (*net_http_req_on_res_begin_fun_t)(void * ctx, net_http_req_t req, uint16_t code, const char * msg);
typedef net_http_res_op_result_t (*net_http_req_on_res_head_fun_t)(void * ctx, net_http_req_t req, const char * name, const char * value);
typedef net_http_res_op_result_t (*net_http_req_on_res_body_fun_t)(void * ctx, net_http_req_t req);
typedef net_http_res_op_result_t (*net_http_req_on_res_complete)(void * ctx, net_http_req_t req);

NET_END_DECL

#endif
