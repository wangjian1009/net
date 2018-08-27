#ifndef NET_HTTP_TYPES_H_INCLEDED
#define NET_HTTP_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_http_protocol * net_http_protocol_t;
typedef struct net_http_endpoint * net_http_endpoint_t;

typedef enum net_http_state {
    net_http_state_disable,
    net_http_state_connecting,
    net_http_state_established,
    net_http_state_error,
} net_http_state_t;

typedef int (*net_http_endpoint_input_fun_t)(net_http_endpoint_t http_ep);

NET_END_DECL

#endif
