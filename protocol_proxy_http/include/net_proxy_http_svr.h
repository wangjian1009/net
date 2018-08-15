#ifndef NET_PROXY_HTTP_SVR_H_INCLEDED
#define NET_PROXY_HTTP_SVR_H_INCLEDED
#include "net_proxy_http_types.h"

NET_BEGIN_DECL

net_proxy_http_svr_t net_proxy_http_svr_create(net_schedule_t schedule);
void net_proxy_http_svr_free(net_proxy_http_svr_t proxy_http_svr);

net_proxy_http_svr_t net_proxy_http_svr_find(net_schedule_t schedule);

NET_END_DECL

#endif
