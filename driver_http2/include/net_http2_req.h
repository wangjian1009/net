#ifndef NET_HTTP2_REQ_H_INCLEDED
#define NET_HTTP2_REQ_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_req_state {
    net_http2_req_state_init,
    net_http2_req_state_connecting,
    net_http2_req_state_head,
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

net_http2_req_t net_http2_req_create(net_http2_endpoint_t http_ep);
void net_http2_req_free(net_http2_req_t req);

net_http2_req_t net_http2_req_find(net_http2_endpoint_t http_ep, uint16_t req_id);

uint16_t net_http2_req_id(net_http2_req_t req);
net_http2_endpoint_t net_http2_req_endpoint(net_http2_req_t req);
net_http2_stream_t net_http2_req_stream(net_http2_req_t req);
net_http2_req_state_t net_http2_req_state(net_http2_req_t req);

/*请求*/
int net_http2_req_add_req_head(net_http2_req_t http_req, const char * attr_name, const char * attr_value);
const char * net_http2_req_find_req_header(net_http2_req_t req, const char * name);

int net_http2_req_start(net_http2_req_t http_req);
int net_http2_req_append(net_http2_req_t http_req, void const * data, uint32_t data_len, uint8_t have_follow_data);

/*请求处理 */
typedef uint8_t (*net_http2_req_on_write_fun_t)(void * ctx, net_http2_req_t req);

uint8_t net_http2_req_is_writing(net_http2_req_t http_req);

int net_http2_req_write(
    net_http2_req_t http_req,
    void * write_ctx, net_http2_req_on_write_fun_t on_write, void (*write_ctx_free)(void *),
    uint8_t have_follow_data);

/*响应处理 */
typedef void (*net_http2_req_on_state_change_fun_t)(void * ctx, net_http2_req_t req, net_http2_req_state_t old_state);
typedef void (*net_http2_req_on_recv_fun_t)(void * ctx, net_http2_req_t req, void const * data, uint32_t data_len);

int net_http2_req_set_reader(
    net_http2_req_t req,
    void * read_ctx,
    net_http2_req_on_state_change_fun_t on_state_change,
    net_http2_req_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *));

void net_http2_req_clear_reader(net_http2_req_t req);

const char * net_http2_req_find_res_header(net_http2_req_t req, const char * name);

const char *  net_http2_req_method_str(net_http2_req_method_t method);
const char * net_http2_req_state_str(net_http2_req_state_t req_state);

NET_END_DECL

#endif
