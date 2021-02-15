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
    union {
        struct {
            net_http2_req_t m_req;
        } m_cli;
        struct {
            net_http2_processor_t m_processor;
        } m_svr;
    };
};

net_http2_stream_t
net_http2_stream_create(
    net_http2_endpoint_t endpoint, uint32_t stream_id, net_http2_stream_runing_mode_t runing_mode);

void net_http2_stream_free(net_http2_stream_t stream);

/*recv*/
int net_http2_stream_on_input(net_http2_stream_t stream, const uint8_t * data, uint32_t len);

void net_http2_stream_on_head_complete(net_http2_stream_t stream);

int net_http2_stream_on_response_head(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

int net_http2_stream_on_tailer(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

NET_END_DECL

#endif
