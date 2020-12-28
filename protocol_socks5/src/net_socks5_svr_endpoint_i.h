#ifndef NET_SOCKS5_SVR_ENDPOINT_I_H_INCLEDED
#define NET_SOCKS5_SVR_ENDPOINT_I_H_INCLEDED
#include "net_socks5_svr_endpoint.h"
#include "net_socks5_svr_i.h"

NET_BEGIN_DECL

typedef enum net_socks5_svr_endpoint_stage {
    net_socks5_svr_endpoint_stage_error = -1      /* Error detected                   */
    , net_socks5_svr_endpoint_stage_init = 0      /* Initial stage                    */
    , net_socks5_svr_endpoint_stage_handshake = 1 /* Handshake with client            */
    , net_socks5_svr_endpoint_stage_parse = 2     /* Parse the SOCKS5 header          */
    , net_socks5_svr_endpoint_stage_sni = 3       /* Parse HTTP/SNI header            */
    , net_socks5_svr_endpoint_stage_resolve = 4   /* Resolve the hostname             */
    , net_socks5_svr_endpoint_stage_stream = 5    /* Stream between client and server */
} net_socks5_svr_endpoint_stage_t;

struct net_socks5_svr_endpoint {
    net_socks5_svr_endpoint_stage_t m_stage;
    net_socks5_svr_connect_fun_t m_on_connect_fun;
    void * m_on_connect_ctx;
};

int net_socks5_svr_endpoint_init(net_endpoint_t endpoint);
void net_socks5_svr_endpoint_fini(net_endpoint_t endpoint);
int net_socks5_svr_endpoint_input(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

NET_END_DECL

#endif
