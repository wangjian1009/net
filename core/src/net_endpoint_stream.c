#include "net_endpoint_stream.h"
#include "net_endpoint_i.h"

int net_endpoint_stream_do_write_to_wbuf(struct write_stream * stream, const void * buf, size_t size) {
    struct net_endpoint_write_stream * ws = (struct net_endpoint_write_stream *)stream;
    if (net_endpoint_wbuf_append(ws->m_endpoint, buf, (uint32_t)size) != 0) return -1;
    return (int)size;
}

void net_endpoint_stream_do_write_to_wbuf_init(struct net_endpoint_write_stream * stream, net_endpoint_t endpoint) {
    stream->m_stream.write = net_endpoint_stream_do_write_to_wbuf;
    stream->m_stream.flush = stream_do_flush_dummy;

    stream->m_endpoint = endpoint;
}

int net_endpoint_stream_do_write_to_fbuf(struct write_stream * stream, const void * buf, size_t size) {
    struct net_endpoint_write_stream * ws = (struct net_endpoint_write_stream *)stream;
    if (net_endpoint_fbuf_append(ws->m_endpoint, buf, (uint32_t)size) != 0) return -1;
    return (int)size;
}

void net_endpoint_stream_do_write_to_fbuf_init(struct net_endpoint_write_stream * stream, net_endpoint_t endpoint) {
    stream->m_stream.write = net_endpoint_stream_do_write_to_fbuf;
    stream->m_stream.flush = stream_do_flush_dummy;

    stream->m_endpoint = endpoint;
}
