#ifndef NET_HTTP2_STREAM_H_INCLEDED
#define NET_HTTP2_STREAM_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_stream_runing_mode {
    net_http2_stream_runing_mode_cli,
    net_http2_stream_runing_mode_svr,
};

uint32_t net_http2_stream_id(net_http2_stream_t stream);
net_http2_stream_runing_mode_t net_http2_stream_runing_mode(net_http2_stream_t stream);
net_http2_endpoint_t net_http2_stream_endpoint(net_http2_stream_t stream);

uint8_t net_http2_stream_read_closed(net_http2_stream_t stream);
uint8_t net_http2_stream_write_closed(net_http2_stream_t stream);

net_http2_req_t net_http2_stream_req(net_http2_stream_t stream);

net_http2_processor_t net_http2_stream_processor(net_http2_stream_t stream);

const char * net_http2_stream_runing_mode_str(net_http2_stream_runing_mode_t runing_mode);

NET_END_DECL

#endif
