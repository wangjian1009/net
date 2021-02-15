#ifndef NET_HTTP2_PROCESSOR_H_INCLEDED
#define NET_HTTP2_PROCESSOR_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_processor_state {
    net_http2_processor_state_init,
    net_http2_processor_state_established,
    net_http2_processor_state_done,
};

net_http2_processor_state_t net_http2_processor_state(net_http2_processor_t processor);
net_http2_endpoint_t net_http2_processor_endpoint(net_http2_processor_t processor);
net_http2_stream_t net_http2_processor_stream(net_http2_processor_t processor);

const char * net_http2_processor_state_str(net_http2_processor_state_t state);

NET_END_DECL

#endif
