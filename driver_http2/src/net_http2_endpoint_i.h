#ifndef NET_HTTP2_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_ENDPOINT_I_H_INCLEDED
#include "nghttp2/nghttp2.h"
#include "net_http2_endpoint.h"
#include "net_http2_protocol_i.h"

struct net_http2_endpoint {
    net_endpoint_t m_base_endpoint;
    nghttp2_session * m_http2_session;
    net_http2_stream_endpoint_list_t m_streams;
};

int net_http2_endpoint_init(net_endpoint_t base_endpoint);
void net_http2_endpoint_fini(net_endpoint_t base_endpoint);
int net_http2_endpoint_input(net_endpoint_t base_endpoint);
int net_http2_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

#endif
