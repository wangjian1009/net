#ifndef NET_HTTP_ENDPOINT_I_H_INCLEDED
#define NET_HTTP_ENDPOINT_I_H_INCLEDED
#include "net_http_endpoint.h"
#include "net_http_protocol_i.h"

NET_BEGIN_DECL

struct net_http_endpoint {
    net_endpoint_t m_endpoint;
    net_http_ssl_ctx_t m_ssl_ctx;
    net_http_connection_type_t m_connection_type;
    uint32_t m_reconnect_span_ms;

    /*runtime*/
    net_http_state_t m_state;
    net_timer_t m_connect_timer;
    net_timer_t m_process_timer;

    uint16_t m_max_req_id;
    net_http_req_list_t m_reqs;

    void * m_write_buf;
    uint32_t m_write_size;
    uint32_t m_write_capacity;
};

int net_http_endpoint_init(net_endpoint_t endpoint);
void net_http_endpoint_fini(net_endpoint_t endpoint);
int net_http_endpoint_input(net_endpoint_t endpoint);
int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

int net_http_endpoint_set_state(net_http_endpoint_t http_ep, net_http_state_t state);

int net_http_endpoint_write(
    net_http_protocol_t http_protocol,
    net_http_endpoint_t http_ep, net_http_req_t http_req, void const * data, uint32_t size);

int net_http_endpoint_flush(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t http_req);

NET_END_DECL

#endif
