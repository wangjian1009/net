#include "net_endpoint_stream.h"
#include "net_endpoint_i.h"

int net_endpoint_stream_do_write_to_buf(struct write_stream * stream, const void * buf, size_t size) {
    struct net_endpoint_write_stream * ws = (struct net_endpoint_write_stream *)stream;
    if (net_endpoint_buf_append(ws->m_endpoint, ws->m_buf_type, buf, (uint32_t)size) != 0) return -1;
    return (int)size;
}

void net_endpoint_stream_do_write_to_buf_init(
    struct net_endpoint_write_stream * stream, net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type)
{
    stream->m_stream.write = net_endpoint_stream_do_write_to_buf;
    stream->m_stream.flush = stream_do_flush_dummy;

    stream->m_endpoint = endpoint;
    stream->m_buf_type = buf_type;
}

