#include "net_smux_stream_i.h"

void net_smux_stream_read_resume(net_smux_stream_t stream) {
    
}

void net_smux_stream_set_reader(
    net_smux_stream_t stream,
    void * read_ctx,
    net_smux_stream_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *))
{
    if (stream->m_read_ctx_free) {
        stream->m_read_ctx_free(stream->m_read_ctx);
    }
    
    stream->m_read_ctx = read_ctx;
    stream->m_on_recv = on_recv;
    stream->m_read_ctx_free = read_ctx_free;
}

void net_smux_stream_clear_reader(net_smux_stream_t stream) {
    stream->m_read_ctx = NULL;
    stream->m_on_recv = NULL;
    stream->m_read_ctx_free = NULL;
}

void * net_smux_stream_reader_ctx(net_smux_stream_t stream) {
    return stream->m_read_ctx;
}


