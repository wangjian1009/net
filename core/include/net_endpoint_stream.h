#ifndef NET_ENDPOINT_STREAM_H_INCLEDED
#define NET_ENDPOINT_STREAM_H_INCLEDED
#include "cpe/utils/stream.h"
#include "net_system.h"

NET_BEGIN_DECL

struct net_endpoint_write_stream {
    struct write_stream m_stream;
    net_endpoint_t m_endpoint;
    net_endpoint_buf_type_t m_buf_type;
};

int net_endpoint_stream_do_write_to_buf(struct write_stream * stream, const void * buf, size_t size);
void net_endpoint_stream_do_write_to_buf_init(struct net_endpoint_write_stream * stream, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

#define NET_ENDPOINT_WRITE_STREAM_TO_BUF(__endpoint, __buf_type)       \
    { CPE_WRITE_STREAM_INITIALIZER(net_endpoint_stream_do_write_to_buf, stream_do_flush_dummy), (__endpoint), (__buf_type) }

NET_END_DECL

#endif
