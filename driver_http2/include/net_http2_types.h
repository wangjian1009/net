#ifndef NET_HTTP2_TYPES_H_INCLEDED
#define NET_HTTP2_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_http2_endpoint_runing_mode net_http2_endpoint_runing_mode_t;
typedef enum net_http2_endpoint_state net_http2_endpoint_state_t;

typedef struct net_http2_protocol * net_http2_protocol_t;
typedef struct net_http2_endpoint * net_http2_endpoint_t;
typedef struct net_http2_endpoint_stream * net_http2_endpoint_stream_t;

typedef struct net_http2_stream_driver * net_http2_stream_driver_t;
typedef struct net_http2_stream_remote * net_http2_stream_remote_t;
typedef struct net_http2_stream_endpoint * net_http2_stream_endpoint_t;
typedef struct net_http2_stream_acceptor * net_http2_stream_acceptor_t;

NET_END_DECL

#endif
