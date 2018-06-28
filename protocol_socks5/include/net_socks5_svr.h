#ifndef NET_SOCKS5_SVR_H_INCLEDED
#define NET_SOCKS5_SVR_H_INCLEDED
#include "net_socks5_types.h"

NET_BEGIN_DECL

net_socks5_svr_t net_socks5_svr_create(
    net_schedule_t schedule,
    net_socks5_svr_connect_fun_t dft_connect, void * dft_connect_ctx);
void net_socks5_svr_free(net_socks5_svr_t socks5_svr);

NET_END_DECL

#endif
