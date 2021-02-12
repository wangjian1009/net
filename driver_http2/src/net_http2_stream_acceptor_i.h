#ifndef NET_HTTP2_STREAM_ACCEPTOR_I_H_INCLEDED
#define NET_HTTP2_STREAM_ACCEPTOR_I_H_INCLEDED
#include "net_http2_stream_acceptor.h"
#include "net_http2_stream_driver_i.h"

struct net_http2_stream_acceptor {
    net_acceptor_t m_control_acceptor;
    net_http2_stream_using_list_t m_usings;
};

int net_http2_stream_acceptor_init(net_acceptor_t acceptor);
void net_http2_stream_acceptor_fini(net_acceptor_t acceptor);

net_http2_stream_endpoint_t
net_http2_stream_acceptor_accept(
    net_http2_stream_acceptor_t acceptor, net_http2_endpoint_t control, int32_t stream_id);

int net_http2_stream_acceptor_established(
    net_http2_stream_acceptor_t acceptor, net_http2_stream_endpoint_t stream);

#endif
