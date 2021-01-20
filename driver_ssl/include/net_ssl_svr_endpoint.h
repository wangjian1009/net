#ifndef NET_SSL_SVR_ENDPOINT_H_INCLEDED
#define NET_SSL_SVR_ENDPOINT_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_endpoint_t
net_ssl_svr_endpoint_create(
    net_ssl_svr_driver_t driver, net_protocol_t protocol);

net_endpoint_t net_ssl_svr_endpoint_underline(net_endpoint_t ssl_endpoint);

NET_END_DECL

#endif
