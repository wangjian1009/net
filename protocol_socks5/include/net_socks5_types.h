#ifndef NET_SOCKS5_TYPES_H_INCLEDED
#define NET_SOCKS5_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_socks5_svr * net_socks5_svr_t;
typedef struct net_socks5_svr_endpoint * net_socks5_svr_endpoint_t;

typedef int (*net_socks5_svr_connect_fun_t)(void * ctx, net_endpoint_t endpoint, net_address_t target);

NET_END_DECL

#endif
