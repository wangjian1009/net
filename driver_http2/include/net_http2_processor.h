#ifndef NET_HTTP2_PROCESSOR_H_INCLEDED
#define NET_HTTP2_PROCESSOR_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

net_http2_processor_t net_http2_processor_create(net_http2_stream_t stream);
void net_http2_processor_free(net_http2_processor_t processor);
    
NET_END_DECL

#endif
