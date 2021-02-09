#ifndef NET_HTTP2_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_ENDPOINT_I_H_INCLEDED
#include "nghttp2/nghttp2.h"
#include "net_http2_endpoint.h"
#include "net_http2_protocol_i.h"

struct net_http2_endpoint {
    net_endpoint_t m_base_endpoint;
    net_http2_endpoint_runing_mode_t m_runing_mode;
    nghttp2_session * m_http2_session;
    net_http2_stream_remote_t m_stream_remote;
    TAILQ_ENTRY(net_http2_endpoint) m_next_for_stream_remote;
    net_http2_stream_endpoint_list_t m_streams;
};

int net_http2_endpoint_init(net_endpoint_t base_endpoint);
void net_http2_endpoint_fini(net_endpoint_t base_endpoint);
int net_http2_endpoint_input(net_endpoint_t base_endpoint);
int net_http2_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

void net_http2_endpoint_set_stream_remote(
    net_http2_endpoint_t endpoint, net_http2_stream_remote_t stream_remote);

#endif
