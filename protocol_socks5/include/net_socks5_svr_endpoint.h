#ifndef NET_SOCKS5_SVR_ENDPOINT_H_INCLEDED
#define NET_SOCKS5_SVR_ENDPOINT_H_INCLEDED
#include "net_socks5_types.h"

NET_BEGIN_DECL

void net_socks5_svr_endpoint_set_connect_fun(
    net_socks5_svr_endpoint_t ss_ep,
    net_socks5_svr_connect_fun_t on_connect_fon, void * on_connect_ctx);

NET_END_DECL

#endif
