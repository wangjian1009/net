#ifndef NET_WS_SVR_ENDPOINT_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_svr_endpoint_t net_ws_svr_endpoint_cast(net_endpoint_t endpoint);
net_endpoint_t net_ws_svr_endpoint_stream(net_endpoint_t endpoint);

NET_END_DECL

#endif
