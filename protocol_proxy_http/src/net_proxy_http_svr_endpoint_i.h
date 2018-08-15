#ifndef NET_PROXY_HTTP_SVR_ENDPOINT_I_H_INCLEDED
#define NET_PROXY_HTTP_SVR_ENDPOINT_I_H_INCLEDED
#include "net_proxy_http_svr_endpoint.h"
#include "net_proxy_http_svr_protocol_i.h"

NET_BEGIN_DECL

typedef enum net_proxy_http_svr_endpoint_stage {
    net_proxy_http_svr_endpoint_stage_error = -1      /* Error detected                   */
    , net_proxy_http_svr_endpoint_stage_init = 0      /* Initial stage                    */
    , net_proxy_http_svr_endpoint_stage_handshake = 1 /* Handshake with client            */
    , net_proxy_http_svr_endpoint_stage_parse = 2     /* Parse the PROXY_HTTP header          */
    , net_proxy_http_svr_endpoint_stage_sni = 3       /* Parse HTTP/SNI header            */
    , net_proxy_http_svr_endpoint_stage_resolve = 4   /* Resolve the hostname             */
    , net_proxy_http_svr_endpoint_stage_stream = 5    /* Stream between client and server */
} net_proxy_http_svr_endpoint_stage_t;

struct net_proxy_http_svr_endpoint {
    net_proxy_http_svr_endpoint_stage_t m_stage;
    net_proxy_http_svr_connect_fun_t m_on_connect_fun;
    void * m_on_connect_ctx;
};

int net_proxy_http_svr_endpoint_init(net_endpoint_t endpoint);
void net_proxy_http_svr_endpoint_fini(net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_input(net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from);

NET_END_DECL

#endif
