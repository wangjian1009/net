#ifndef NET_PROXY_HTTP_SVR_ENDPOINT_H_INCLEDED
#define NET_PROXY_HTTP_SVR_ENDPOINT_H_INCLEDED
#include "net_proxy_http_types.h"

NET_BEGIN_DECL

net_proxy_http_way_t net_proxy_http_svr_endpoint_way(net_proxy_http_svr_endpoint_t http_ep);

void net_proxy_http_svr_endpoint_set_connect_fun(
    net_proxy_http_svr_endpoint_t http_ep,
    net_proxy_http_svr_connect_fun_t on_connect_fon, void * on_connect_ctx);

NET_END_DECL

#endif
