#ifndef NET_HTTP2_STREAM_ACCEPTOR_I_H_INCLEDED
#define NET_HTTP2_STREAM_ACCEPTOR_I_H_INCLEDED
#include "net_http2_stream_acceptor.h"
#include "net_http2_stream_driver_i.h"
#include "net_http2_protocol_i.h"

struct net_http2_stream_acceptor {
    net_acceptor_t m_base_acceptor;
    net_http2_endpoint_list_t m_endpoints;
};

int net_http2_stream_acceptor_init(net_acceptor_t acceptor);
void net_http2_stream_acceptor_fini(net_acceptor_t acceptor);

#endif
