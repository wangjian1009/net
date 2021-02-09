#ifndef NET_HTTP2_ENDPOINT_STREAM_I_H_INCLEDED
#define NET_HTTP2_ENDPOINT_STREAM_I_H_INCLEDED
#include "net_http2_endpoint_stream.h"
#include "net_http2_endpoint_i.h"

struct net_http2_endpoint_stream {
    net_http2_endpoint_t m_endpoint;
    int32_t m_stream_id;
};

int net_http2_endpoint_stream_on_head(
    net_http2_endpoint_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

int net_http2_endpoint_stream_on_tailer(
    net_http2_endpoint_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

#endif
