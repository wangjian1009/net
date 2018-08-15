#ifndef NET_PROXY_HTTP_TYPES_H_INCLEDED
#define NET_PROXY_HTTP_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_proxy_http_svr_protocol * net_proxy_http_svr_protocol_t;
typedef struct net_proxy_http_svr_endpoint * net_proxy_http_svr_endpoint_t;

typedef int (*net_proxy_http_svr_connect_fun_t)(void * ctx, net_endpoint_t endpoint, net_address_t target, uint8_t is_own);

NET_END_DECL

#endif
