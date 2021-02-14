#ifndef NET_HTTP2_PROCESSOR_I_H_INCLEDED
#define NET_HTTP2_PROCESSOR_I_H_INCLEDED
#include "net_http2_processor.h"
#include "net_http2_endpoint_i.h"

struct net_http2_processor {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_req) m_next;
    net_http2_stream_t m_stream;
};

void net_http2_processor_set_stream(net_http2_processor_t processor, net_http2_stream_t stream);

#endif
