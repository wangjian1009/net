#ifndef NET_HTTP_TYPES_H_INCLEDED
#define NET_HTTP_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_http_protocol * net_http_protocol_t;
typedef struct net_http_endpoint * net_http_endpoint_t;
typedef struct net_http_req * net_http_req_t;
typedef struct net_http_req_it * net_http_req_it_t;

typedef enum net_http_connection_type {
    net_http_connection_type_keep_alive,
    net_http_connection_type_close,
    net_http_connection_type_upgrade
} net_http_connection_type_t;

typedef enum net_http_transfer_encoding {
    net_http_transfer_identity,
    net_http_transfer_chunked,
} net_http_transfer_encoding_t;

typedef enum net_http_req_state {
    net_http_req_state_prepare_head,
    net_http_req_state_prepare_body,
    net_http_req_state_completed,
} net_http_req_state_t;

typedef enum net_http_req_method {
    net_http_req_method_get,
    net_http_req_method_post,
    net_http_req_method_put,
    net_http_req_method_delete,
    net_http_req_method_patch,
    net_http_req_method_head,
} net_http_req_method_t;

NET_END_DECL

#endif
