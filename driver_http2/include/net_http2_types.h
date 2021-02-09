#ifndef NET_HTTP2_TYPES_H_INCLEDED
#define NET_HTTP2_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_http2_control_protocol * net_http2_control_protocol_t;
typedef struct net_http2_control_endpoint * net_http2_control_endpoint_t;
typedef struct net_http2_driver * net_http2_driver_t;
typedef struct net_http2_endpoint * net_http2_endpoint_t;
typedef struct net_http2_acceptor * net_http2_acceptor_t;

NET_END_DECL

#endif
