#ifndef NET_HTTP2_STREAM_ENDPOINT_H_INCLEDED
#define NET_HTTP2_STREAM_ENDPOINT_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_stream_runing_mode {
    net_http2_stream_runing_mode_cli,
    net_http2_stream_runing_mode_svr,
};

net_http2_stream_runing_mode_t net_http2_stream_runing_mode(net_http2_stream_t stream);

const char * net_http2_stream_runing_mode_str(net_http2_stream_runing_mode_t runing_mode);

NET_END_DECL

#endif
