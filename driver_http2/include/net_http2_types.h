#ifndef NET_HTTP2_TYPES_H_INCLEDED
#define NET_HTTP2_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

/*http2*/
typedef enum net_http2_endpoint_runing_mode net_http2_endpoint_runing_mode_t;
typedef enum net_http2_endpoint_state net_http2_endpoint_state_t;

typedef enum net_http2_req_state net_http2_req_state_t;
typedef enum net_http2_req_method net_http2_req_method_t;
typedef enum net_http2_res_state net_http2_res_state_t;

typedef struct net_http2_protocol * net_http2_protocol_t;
typedef struct net_http2_endpoint * net_http2_endpoint_t;
typedef struct net_http2_stream * net_http2_stream_t;
typedef struct net_http2_req * net_http2_req_t;
typedef struct net_http2_processor * net_http2_processor_t;

/*stream*/
typedef enum net_http2_stream_endpoint_state net_http2_stream_endpoint_state_t;
typedef struct net_http2_stream_driver * net_http2_stream_driver_t;
typedef struct net_http2_stream_group * net_http2_stream_group_t;
typedef struct net_http2_stream_endpoint * net_http2_stream_endpoint_t;
typedef struct net_http2_stream_acceptor * net_http2_stream_acceptor_t;

NET_END_DECL

#endif
