#ifndef NET_HTTP_SVR_SYSTEM_H
#define NET_HTTP_SVR_SYSTEM_H
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_http_svr_protocol * net_http_svr_protocol_t;
typedef struct net_http_svr_endpoint * net_http_svr_endpoint_t;
typedef struct net_http_svr_request * net_http_svr_request_t;
typedef struct net_http_svr_request_header * net_http_svr_request_header_t;
typedef struct net_http_svr_request_header_it * net_http_svr_request_header_it_t;
typedef struct net_http_svr_response * net_http_svr_response_t;
typedef struct net_http_svr_mount_point * net_http_svr_mount_point_t;
typedef struct net_http_svr_processor * net_http_svr_processor_t;

typedef enum net_http_svr_request_method net_http_svr_request_method_t;
typedef enum net_http_svr_request_transfer_encoding net_http_svr_request_transfer_encoding_t;
typedef enum net_http_svr_request_state net_http_svr_request_state_t;
typedef enum net_http_svr_response_state net_http_svr_response_state_t;

NET_END_DECL

#endif
