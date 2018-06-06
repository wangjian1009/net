#ifndef NET_ENDPOINT_STREAM_H_INCLEDED
#define NET_ENDPOINT_STREAM_H_INCLEDED
#include "cpe/utils/stream.h"
#include "net_system.h"

NET_BEGIN_DECL

struct net_endpoint_write_stream {
    struct write_stream m_stream;
    net_endpoint_t m_endpoint;
};

int net_endpoint_stream_do_write_to_wbuf(struct write_stream * stream, const void * buf, size_t size);
void net_endpoint_stream_do_write_to_wbuf_init(struct net_endpoint_write_stream * stream, net_endpoint_t endpoint);

#define NET_ENDPOINT_WRITE_STREAM_TO_WBUF(__endpoint) \
    { CPE_WRITE_STREAM_INITIALIZER(net_endpoint_stream_do_write_to_wbuf, stream_do_flush_dummy), __endpoint }


int net_endpoint_stream_do_write_to_fbuf(struct write_stream * stream, const void * buf, size_t size);
void net_endpoint_stream_do_write_to_fbuf_init(struct net_endpoint_write_stream * stream, net_endpoint_t endpoint);

#define NET_ENDPOINT_WRITE_STREAM_TO_FBUF(__endpoint) \
    { CPE_WRITE_STREAM_INITIALIZER(net_endpoint_stream_do_write_to_fbuf, stream_do_flush_dummy), __endpoint }

NET_END_DECL

#endif
