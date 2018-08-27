#ifndef NET_HTTP_TYPES_H_INCLEDED
#define NET_HTTP_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_http_protocol * net_http_protocol_t;
typedef struct net_http_endpoint * net_http_endpoint_t;
typedef struct net_http_req * net_http_req_t;

typedef enum net_http_state {
    net_http_state_disable,
    net_http_state_connecting,
    net_http_state_established,
    net_http_state_error,
} net_http_state_t;

typedef int (*net_http_endpoint_input_fun_t)(net_http_endpoint_t http_ep);

typedef enum net_http_req_state {
    net_http_req_state_prepare_head,
    net_http_req_state_prepare_body,
    net_http_req_state_completed,
} net_http_req_state_t;

typedef enum net_http_req_method {
    net_http_req_method_get,
    net_http_req_method_post,
} net_http_req_method_t;

typedef enum net_http_res_state {
    net_http_res_state_init,
    net_http_res_state_reading_head,
    net_http_res_state_reading_body,
    net_http_res_state_completed,
} net_http_res_state_t;

NET_END_DECL

#endif
