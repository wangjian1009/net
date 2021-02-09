#ifndef NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#include "net_http2_stream_endpoint.h"
#include "net_http2_stream_driver_i.h"

struct net_http2_stream_endpoint {
    net_endpoint_t m_base_endpoint;
    net_http2_endpoint_t m_control;
    int32_t m_stream_id;
};

int net_http2_stream_endpoint_init(net_endpoint_t endpoint);
void net_http2_stream_endpoint_fini(net_endpoint_t endpoint);
int net_http2_stream_endpoint_connect(net_endpoint_t endpoint);
int net_http2_stream_endpoint_update(net_endpoint_t endpoint);
int net_http2_stream_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_http2_stream_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

int net_http2_stream_endpoint_on_head(
    net_http2_stream_endpoint_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

int net_http2_stream_endpoint_on_tailer(
    net_http2_stream_endpoint_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len);

#endif
