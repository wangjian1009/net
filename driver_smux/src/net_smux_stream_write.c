#include <assert.h>
#include "net_smux_stream_i.h"
#include "net_smux_pro.h"

int net_smux_stream_write_v1(net_smux_stream_t stream, void const * data, uint32_t data_len) {
    net_smux_session_t session = stream->m_session;
    net_smux_protocol_t protocol = session->m_protocol;

    uint32_t total_send = 0;

    uint32_t frame_size = session->m_config->m_max_frame_size;
    
    while(data_len > 0) {
        uint32_t send_len = data_len > frame_size ? frame_size : data_len;
        if (net_smux_session_send_frame(
                session, stream,
                net_smux_cmd_psh, data, send_len,
                0, stream->m_num_written) != 0)
        {
            return -1;
        }
        stream->m_num_written++;
        total_send += send_len;
        data_len -= send_len;
        data = ((uint8_t const *)data) + send_len;
	}
    
    return total_send;
}

int net_smux_stream_write_v2(net_smux_stream_t stream, void const * data, uint32_t data_len) {
    net_smux_session_t session = stream->m_session;
    net_smux_protocol_t protocol = session->m_protocol;

    return 0;
}

int net_smux_stream_write(net_smux_stream_t stream, void const * data, uint32_t data_len) {
    net_smux_session_t session = stream->m_session;

    if (session->m_config->m_version == 1) {
        return net_smux_stream_write_v1(stream, data, data_len);
    }
    else {
        assert(session->m_config->m_version == 2);
        return net_smux_stream_write_v2(stream, data, data_len);
    }
}

void net_smux_stream_set_writer(
    net_smux_stream_t stream,
    void * write_ctx,
    net_smux_stream_on_write_resume_fun_t on_write_resume,
    void (*write_ctx_free)(void *))
{
    if (stream->m_write_ctx_free) {
        stream->m_write_ctx_free(stream->m_write_ctx);
    }
    
    stream->m_write_ctx = write_ctx;
    stream->m_on_write_resume = on_write_resume;
    stream->m_write_ctx_free = write_ctx_free;
}

void net_smux_stream_clear_writeer(net_smux_stream_t stream) {
    stream->m_write_ctx = NULL;
    stream->m_on_write_resume = NULL;
    stream->m_write_ctx_free = NULL;
}

void * net_smux_stream_writeer_ctx(net_smux_stream_t stream) {
    return stream->m_write_ctx;
}
