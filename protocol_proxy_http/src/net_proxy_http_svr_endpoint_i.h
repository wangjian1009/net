#ifndef NET_PROXY_HTTP_SVR_ENDPOINT_I_H_INCLEDED
#define NET_PROXY_HTTP_SVR_ENDPOINT_I_H_INCLEDED
#include "net_proxy_http_svr_endpoint.h"
#include "net_proxy_http_svr_protocol_i.h"

NET_BEGIN_DECL

typedef enum proxy_http_svr_basic_trans_encoding {
    proxy_http_svr_basic_trans_encoding_none
    , proxy_http_svr_basic_trans_encoding_trunked
} proxy_http_svr_basic_trans_encoding_t;

typedef enum proxy_http_svr_basic_req_state {
    proxy_http_svr_basic_req_state_header
    , proxy_http_svr_basic_req_state_content
    , proxy_http_svr_basic_req_state_stop
} proxy_http_svr_basic_req_state_t;

typedef enum proxy_http_svr_basic_rsp_state {
    proxy_http_svr_basic_rsp_state_header
    , proxy_http_svr_basic_rsp_state_content
} proxy_http_svr_basic_rsp_state_t;

typedef enum proxy_http_svr_tunnel_state {
    proxy_http_svr_tunnel_state_connecting
    , proxy_http_svr_tunnel_state_established
} proxy_http_svr_tunnel_state_t;

struct net_proxy_http_svr_endpoint {
    uint16_t m_max_head_len;
    net_proxy_http_way_t m_way;
    union {
        struct {
            proxy_http_svr_tunnel_state_t m_state;
            net_endpoint_monitor_t m_other_state_monitor;
        } m_tunnel;
        struct {
            struct {
                proxy_http_svr_basic_req_state_t m_state;
                proxy_http_svr_basic_trans_encoding_t m_trans_encoding;
                union {
                    struct {
                        uint32_t m_length;
                    } m_content;
                    struct {
                        uint16_t m_count;
                        uint32_t m_length;
                    } m_trunked;
                };
            } m_req;
            struct {
                proxy_http_svr_basic_rsp_state_t m_state;
                proxy_http_svr_basic_trans_encoding_t m_trans_encoding;
                union {
                    struct {
                        uint32_t m_length;
                    } m_content;
                    struct {
                        uint16_t m_count;
                        uint32_t m_length;
                    } m_trunked;
                };
            } m_rsp;
        } m_basic;
    };
    uint8_t m_keep_alive;
    net_proxy_http_svr_connect_fun_t m_on_connect_fun;
    void * m_on_connect_ctx;
};

int net_proxy_http_svr_endpoint_init(net_endpoint_t endpoint);
void net_proxy_http_svr_endpoint_fini(net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_input(net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from);

/*basic*/
int net_proxy_http_svr_endpoint_basic_req_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data);
int net_proxy_http_svr_endpoint_basic_forward(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_basic_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

/*tunnel*/
int net_proxy_http_svr_endpoint_tunnel_on_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * request);
int net_proxy_http_svr_endpoint_tunnel_forward(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);
int net_proxy_http_svr_endpoint_tunnel_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

NET_END_DECL

#endif
