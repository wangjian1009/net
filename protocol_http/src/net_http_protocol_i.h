#ifndef NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#define NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_http_protocol.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_http_req_list, net_http_req) net_http_req_list_t;
typedef TAILQ_HEAD(net_http_ssl_ctx_list, net_http_ssl_ctx) net_http_ssl_ctx_list_t;

struct net_http_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;

    /*protocol*/
    uint16_t m_protocol_capacity;
    net_http_protocol_init_fun_t m_protocol_init;
    net_http_protocol_fini_fun_t m_protocol_fini;
    /*endpoint*/
    uint16_t m_endpoint_capacity;
    net_http_endpoint_init_fun_t m_endpoint_init;
    net_http_endpoint_fini_fun_t m_endpoint_fini;
    net_http_endpoint_init_fun_t m_endpoint_upgraded_input;
    net_http_endpoint_on_state_change_fun_t m_endpoint_on_state_change;

    /*runtime*/
    net_http_req_list_t m_free_reqs;
    net_http_ssl_ctx_list_t m_free_ssl_ctxes;
};

mem_buffer_t net_http_protocol_tmp_buffer(net_http_protocol_t ws_protocol);
net_schedule_t net_http_protocol_schedule(net_http_protocol_t ws_protocol);

NET_END_DECL

#endif
