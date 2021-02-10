#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_utils.h"

ssize_t net_http2_endpoint_send_callback(
    nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_buf_size(endpoint->m_base_endpoint, net_ep_buf_write) >= OUTPUT_WOULDBLOCK_THRESHOLD) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: net: => write buf size %d, threahold %d reached, would block",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                net_endpoint_buf_size(endpoint->m_base_endpoint, net_ep_buf_write),
                OUTPUT_WOULDBLOCK_THRESHOLD);
        }

        return NGHTTP2_ERR_WOULDBLOCK;
    }

    int rv = net_endpoint_buf_append(endpoint->m_base_endpoint, net_ep_buf_write, data, length);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: net: ==> write %d data fail",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            (int)length);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: net: ==> net %d bytes",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            (int)length);
    }

    return (ssize_t)length;
}

int net_http2_endpoint_on_frame_recv_callback(
    nghttp2_session * session, const nghttp2_frame * frame, void * user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        mem_buffer_t buffer = net_http2_protocol_tmp_buffer(protocol);
        mem_buffer_clear_data(buffer);

        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

        stream_printf((write_stream_t)&ws, "http2: ");
        net_endpoint_print((write_stream_t)&ws, endpoint->m_base_endpoint);
        stream_printf(
            (write_stream_t)&ws, ": %s: http2: ",
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode));

        if (frame->hd.stream_id) {
            stream_printf((write_stream_t)&ws, "%d: ", frame->hd.stream_id);
        }
        
        stream_printf((write_stream_t)&ws, "<== ");
        net_http2_print_frame((write_stream_t)&ws, frame);
        stream_putc((write_stream_t)&ws, 0);

        CPE_INFO(protocol->m_em, "%s", (const char *)mem_buffer_make_continuous(buffer, 0));
    }

    return 0;
}

int net_http2_endpoint_on_frame_schedule_next_send(
    nghttp2_session * session, const nghttp2_frame * frame, net_http2_endpoint_t endpoint)
{
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_endpoint_t stream;
    
    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
            stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (stream == NULL) {
                CPE_ERROR(
                    protocol->m_em, "http2: %s: %s: http2: %d: <== recv end stream, no bind stream",
                    net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                    net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                    frame->hd.stream_id);
                return 0;
            }

            if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
            }
            return 0;
        }

        if (frame->hd.flags & NGHTTP2_FLAG_END_HEADERS) {
            goto CHECK_NEXT_SEND;
        }
        break;
    case NGHTTP2_DATA:
        goto CHECK_NEXT_SEND;
    default:
        break;
    }
    
    return 0;

CHECK_NEXT_SEND:
    stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> on frame send: no bind stream",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    if (net_endpoint_state(stream->m_base_endpoint) == net_endpoint_state_established) {
        assert(net_endpoint_is_writing(stream->m_base_endpoint));
        assert(!stream->m_send_scheduled);

        if (net_endpoint_buf_is_empty(stream->m_base_endpoint, net_ep_buf_write)) {
            net_endpoint_set_is_writing(stream->m_base_endpoint, 0);
        }
        else {
            //net_http2_stream_endpoint_schedule_send_data(stream);
        }
    }

    return 0;
}

int net_http2_endpoint_on_frame_send_callback(
    nghttp2_session * session, const nghttp2_frame * frame, void * user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        mem_buffer_t buffer = net_http2_protocol_tmp_buffer(protocol);
        mem_buffer_clear_data(buffer);

        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

        stream_printf((write_stream_t)&ws, "http2: ");
        net_endpoint_print((write_stream_t)&ws, endpoint->m_base_endpoint);
        stream_printf(
            (write_stream_t)&ws, ": %s: http2: ",
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode));
        
        if (frame->hd.stream_id) {
            stream_printf((write_stream_t)&ws, "%d: ", frame->hd.stream_id);
        }

        stream_printf((write_stream_t)&ws, "==> ");
        net_http2_print_frame((write_stream_t)&ws, frame);
        stream_putc((write_stream_t)&ws, 0);

        CPE_INFO(protocol->m_em, "%s", (const char *)mem_buffer_make_continuous(buffer, 0));
    }

    return net_http2_endpoint_on_frame_schedule_next_send(session, frame, endpoint);
}

int net_http2_endpoint_on_frame_not_send_callback(
    nghttp2_session * session, const nghttp2_frame * frame, int lib_error_code,  void * user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        mem_buffer_t buffer = net_http2_protocol_tmp_buffer(protocol);
        mem_buffer_clear_data(buffer);

        struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

        stream_printf((write_stream_t)&ws, "http2: ");
        net_endpoint_print((write_stream_t)&ws, endpoint->m_base_endpoint);
        stream_printf((write_stream_t)&ws, ": ");

        if (frame->hd.stream_id) {
            stream_printf((write_stream_t)&ws, "%d: ", frame->hd.stream_id);
        }
        
        stream_printf(
            (write_stream_t)&ws, "xxx error %d (%s) | ",
            lib_error_code, nghttp2_strerror(lib_error_code));
        net_http2_print_frame((write_stream_t)&ws, frame);
        stream_putc((write_stream_t)&ws, 0);

        CPE_INFO(protocol->m_em, "%s", (const char *)mem_buffer_make_continuous(buffer, 0));
    }

    return net_http2_endpoint_on_frame_schedule_next_send(session, frame, endpoint);
}

int net_http2_endpoint_on_data_chunk_recv_callback(
    nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_endpoint_t stream;

    stream = nghttp2_session_get_stream_user_data(session, stream_id);
    if (!stream) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %d: recv data: no bind endpoint",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    if (net_endpoint_buf_append(stream->m_base_endpoint, net_ep_buf_read, data, (uint32_t)len) != 0) {
        if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) {
            net_endpoint_set_error(
                stream->m_base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
        }
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }

    return 0;
}

int net_http2_endpoint_on_stream_close_callback(
    nghttp2_session * session, int32_t stream_id, uint32_t error_code, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    net_http2_stream_endpoint_t stream = nghttp2_session_get_stream_user_data(session, stream_id);
    if (stream == NULL || !net_endpoint_is_active(stream->m_base_endpoint)) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %d: already closed!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                stream_id);
        }
        return 0;
    }

    if (stream->m_stream_id != -1) {
        assert(stream->m_stream_id == stream_id);
        if (net_endpoint_protocol_debug(stream->m_base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %d: stream close and ignore rst!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), stream->m_base_endpoint),
                stream->m_stream_id);
        }
        stream->m_stream_id = -1;
    }
    
    if (error_code == 0) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %d: stream closed no error",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                stream_id);
        }

        net_endpoint_set_error(
            stream->m_base_endpoint,
            net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }
    else {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %d: stream closed, error=%s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            stream_id, net_http2_error_code_str(error_code));

        if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) {
            net_endpoint_set_error(
                stream->m_base_endpoint, net_endpoint_error_source_network,
                net_endpoint_network_errno_remote_closed, net_http2_error_code_str(error_code));
        }

        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting);
        }
    }

    return 0;
}

int net_http2_endpoint_on_invalid_header_callback(
    nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags,
    void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    CPE_INFO(
        protocol->m_em, "http2: %s: %s: http2: %d: <== invalid head: %.*s = %.*s",
        net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
        net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
        frame->hd.stream_id, (int)namelen, (const char *)name, (int)valuelen, (const char *)value);

    return 0;
}

int net_http2_endpoint_on_header_callback(
    nghttp2_session *session,
    const nghttp2_frame *frame, const uint8_t *name,
    size_t namelen, const uint8_t *value,
    size_t valuelen, uint8_t flags, void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    assert(frame->hd.type == NGHTTP2_HEADERS);

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: <== %.*s = %.*s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id, (int)namelen, (const char *)name, (int)valuelen, (const char *)value);
    }

    net_http2_stream_endpoint_t stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (!stream) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: <== head no bind endpoint",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
        if (net_http2_stream_endpoint_on_head(
                stream,
                (const char *)name, (uint32_t)namelen, (const char *)value, (uint32_t)valuelen)
            != 0)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }
    else if (frame->headers.cat == NGHTTP2_HCAT_HEADERS) {
        if (net_http2_stream_endpoint_on_tailer(
                stream,
                (const char *)name, (uint32_t)namelen, (const char *)value, (uint32_t)valuelen)
            != 0)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }
    else {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: <== ignore header cat %d",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id, frame->headers.cat);
    }

    return 0;
}

int net_http2_endpoint_on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        /* if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && session_data->data->id == frame->hd.id) { */
        /*     fprintf(stderr, "Response headers for stream ID=%d:\n", */
        /*         frame->hd.id); */
        /* } */
        break;
    }
    return 0;
}

int net_http2_endpoint_on_error_callback(
    nghttp2_session *session,
    int lib_error_code, const char *msg, size_t len, void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    CPE_INFO(
        protocol->m_em, "http2: %s: %s: http2: error: code=%d %.*s",
        net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
        net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
        lib_error_code, (int)len, msg);
    
    return NGHTTP2_NO_ERROR;
}

int net_http2_endpoint_send_data_callback(
    nghttp2_session *session,
    nghttp2_frame *frame, const uint8_t *framehd, size_t length, nghttp2_data_source *source, void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_endpoint_t stream = source->ptr;

    uint32_t head_sz = 9;
    if (frame->data.padlen > 0) head_sz++;

    uint8_t * head_data = net_endpoint_buf_alloc(endpoint->m_base_endpoint, head_sz);
    if (head_data == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> alloc head %d data fail",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, head_sz);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    memcpy(head_data, framehd, 9);
    if (frame->data.padlen > 0) {
        head_data[9] = (frame->data.padlen - 1);
    }
    
    if (net_endpoint_buf_supply(endpoint->m_base_endpoint, net_ep_buf_write, head_sz) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> write head %d data fail",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, head_sz);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    if (net_endpoint_buf_append_from_other(
            endpoint->m_base_endpoint, net_ep_buf_write,
            stream->m_base_endpoint, net_ep_buf_write,
            length) != 0)
    {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> write body %d data fail",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, (int)length);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> %d data",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, (int)head_sz + (int)length);
    }

    return 0;
}
    
int net_http2_endpoint_http2_send_settings(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    nghttp2_settings_entry iv[1] = {
        { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };

    /* client 24 bytes magic string will be sent by nghttp2 library */
    int rv = nghttp2_submit_settings(endpoint->m_http2_session, NGHTTP2_FLAG_NONE, iv, CPE_ARRAY_SIZE(iv));
    if (rv != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: ==> submit settings fail, error=%s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            nghttp2_strerror(rv));
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2 ==> submit settings",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode));
    }
    
    return 0;
}
