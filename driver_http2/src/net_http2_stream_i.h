#ifndef NET_HTTP2_STREAM_I_H_INCLEDED
#define NET_HTTP2_STREAM_I_H_INCLEDED
#include "net_http2_stream.h"
#include "net_http2_endpoint_i.h"

NET_BEGIN_DECL

struct net_http2_stream {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_stream) m_next_for_ep;
    int32_t m_stream_id;
};

net_http2_stream_t net_http2_stream_create(net_http2_endpoint_t endpoint);
void net_http2_stream_free(net_http2_stream_t stream);

/*send*/
int net_http2_stream_commit_headers(net_http2_stream_t stream);

/*recv*/
void net_http2_stream_on_input(net_http2_stream_t stream, const uint8_t * data, uint32_t len);

void net_http2_stream_on_close(net_http2_stream_t stream, int http2_error);

void net_http2_stream_on_head_complete(net_http2_stream_t stream);

int net_http2_stream_on_request_head(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

int net_http2_stream_on_response_head(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

int net_http2_stream_on_tailer(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

NET_END_DECL

#endif
