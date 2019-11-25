#ifndef NET_NGHTTP2_TYPES_H_INCLEDED
#define NET_NGHTTP2_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_nghttp2_service * net_nghttp2_service_t;
typedef struct net_nghttp2_session * net_nghttp2_session_t;
typedef struct net_nghttp2_stream * net_nghttp2_stream_t;
typedef struct net_nghttp2_request * net_nghttp2_request_t;
typedef struct net_nghttp2_request_header * net_nghttp2_request_header_t;
typedef struct net_nghttp2_request_header_it * net_nghttp2_request_header_it_t;
typedef struct net_nghttp2_response * net_nghttp2_response_t;
typedef struct net_nghttp2_mount_point * net_nghttp2_mount_point_t;
typedef struct net_nghttp2_processor * net_nghttp2_processor_t;

typedef enum net_nghttp2_request_method net_nghttp2_request_method_t;
typedef enum net_nghttp2_request_transfer_encoding net_nghttp2_request_transfer_encoding_t;
typedef enum net_nghttp2_request_state net_nghttp2_request_state_t;
typedef enum net_nghttp2_response_state net_nghttp2_response_state_t;

NET_END_DECL

#endif
