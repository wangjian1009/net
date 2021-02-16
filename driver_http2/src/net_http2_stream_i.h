#ifndef NET_HTTP2_STREAM_I_H_INCLEDED
#define NET_HTTP2_STREAM_I_H_INCLEDED
#include "net_http2_stream.h"
#include "net_http2_endpoint_i.h"

NET_BEGIN_DECL

struct net_http2_stream {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_stream) m_next_for_ep;
    uint32_t m_stream_id;
    net_http2_stream_runing_mode_t m_runing_mode;
    uint8_t m_read_closed;
    uint8_t m_write_closed;
    net_http2_req_t m_req;
};

net_http2_stream_t
net_http2_stream_create(net_http2_endpoint_t endpoint, uint32_t stream_id, net_http2_stream_runing_mode_t runing_mode);

void net_http2_stream_free(net_http2_stream_t stream);

NET_END_DECL

#endif
