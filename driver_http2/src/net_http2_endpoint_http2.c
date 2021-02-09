#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_endpoint_stream_i.h"
#include "net_http2_utils.h"

ssize_t net_http2_endpoint_send_callback(
    nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    /* if (evbuffer_get_length(bufferevent_get_output(endpoint->m_bev)) >= OUTPUT_WOULDBLOCK_THRESHOLD) { */
    /*     if (net_endpoint_debug(endpoint->m_base_endpoint) >= 3) { */
    /*         CPE_INFO( */
    /*             protocol->m_em, "http2: %s: http2: send: threahold %d reached, would block", */
    /*             net_endpoint_dump(net_http2_protocol_tmp_buffer(net_http2_protocol), endpoint), */
    /*             (int)OUTPUT_WOULDBLOCK_THRESHOLD); */
    /*     } */

    /*     return NGHTTP2_ERR_WOULDBLOCK; */
    /* } */
    
    /* int rv = bufferevent_write(endpoint->m_bev, data, length); */
    /* if (rv < 0) { */
    /*     CPE_ERROR( */
    /*         net_http2_protocol->m_em, "http2: %s: send: write %d data fail", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(net_http2_protocol), endpoint), */
    /*         (int)length); */
    /*     return NGHTTP2_ERR_CALLBACK_FAILURE; */
    /* } */

    /* if (net_endpoint_debug(endpoint->m_base_endpoint) >= 3) { */
    /*     CPE_INFO( */
    /*         net_http2_protocol->m_em, "http2: %s: http2: ==> net %d bytes", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(net_http2_protocol), endpoint), */
    /*         (int)length); */
    /* } */

    /* return (ssize_t)length; */
    return 0;
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
        stream_printf((write_stream_t)&ws, ": ");

        if (frame->hd.stream_id) {
            stream_printf((write_stream_t)&ws, "%d: ", frame->hd.stream_id);
        }
        
        stream_printf((write_stream_t)&ws, "   <==       ");
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
    /* net_http2_endpoint_stream_t sfox_ep; */
    
    /* switch (frame->hd.type) { */
    /* case NGHTTP2_HEADERS: */
    /*     if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) { */
    /*         sfox_ep = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id); */
    /*         if (sfox_ep == NULL) { */
    /*             CPE_ERROR( */
    /*                 protocol->m_em, "http2: %s: %d: schedule next send: stream closed", */
    /*                 net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint), */
    /*                 frame->hd.stream_id); */
    /*             return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
    /*         } */

    /*         //sfox_client_conn_set_state(sfox_ep->m_conn, sfox_client_conn_state_closed); */
    /*         return 0; */
    /*     } */

    /*     if (frame->hd.flags & NGHTTP2_FLAG_END_HEADERS) { */
    /*         goto CHECK_NEXT_SEND; */
    /*     } */
    /*     break; */
    /* case NGHTTP2_DATA: */
    /*     goto CHECK_NEXT_SEND; */
    /* default: */
    /*     break; */
    /* } */
    
    return 0;

/* CHECK_NEXT_SEND: */
/*     sfox_ep = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id); */
/*     if (sfox_ep == NULL) { */
/*         CPE_ERROR( */
/*             protocol->m_em, "http2: %s: %d: on frame send: no bind endpoint", */
/*             net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint), */
/*             frame->hd.stream_id); */
/*         return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
/*     } */

/*     if (net_endpoint_state(sfox_ep->m_endpoint) == net_endpoint_state_established) { */
/*         assert(net_endpoint_is_writing(sfox_ep->m_endpoint)); */
/*         assert(!sfox_ep->m_send_scheduled); */

/*         if (net_endpoint_buf_is_empty(sfox_ep->m_endpoint, net_ep_buf_write)) { */
/*             net_endpoint_set_is_writing(sfox_ep->m_endpoint, 0); */
/*         } */
/*         else { */
/*             sfox_sfox_cli_endpoint_schedule_send_data(sfox_ep); */
/*         } */
/*     } */

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
        stream_printf((write_stream_t)&ws, ": ");

        if (frame->hd.stream_id) {
            stream_printf((write_stream_t)&ws, "%d: ", frame->hd.stream_id);
        }

        stream_printf((write_stream_t)&ws, "   ==>       ");
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

    //TODO:
    /* net_cli_endpoint_t sfox_ep = nghttp2_session_get_stream_user_data(session, stream_id); */
    /* if (!sfox_ep) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "http2: %s: %d: recv data: no bind endpoint", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint), */
    /*         stream_id); */
    /*     return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
    /* } */

    /* if (net_endpoint_buf_append(sfox_ep->m_endpoint, net_ep_buf_read, data, (uint32_t)len) != 0) { */
    /*     return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE; */
    /* } */

    return 0;
}

int net_http2_endpoint_on_stream_close_callback(
    nghttp2_session * session, int32_t stream_id, uint32_t error_code, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    net_http2_endpoint_stream_t stream = nghttp2_session_get_stream_user_data(session, stream_id);
    if (stream == NULL || !net_endpoint_is_active(stream->m_endpoint->m_base_endpoint)) {
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
        if (net_endpoint_protocol_debug(stream->m_endpoint->m_base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %d: stream close and ignore rst!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), stream->m_endpoint->m_base_endpoint),
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
            stream->m_endpoint->m_base_endpoint,
            net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
        if (net_endpoint_set_state(stream->m_endpoint->m_base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(stream->m_endpoint->m_base_endpoint, net_endpoint_state_deleting);
        }
    }
    else {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %d: stream closed, error=%s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            stream_id, net_http2_error_code_str(error_code));

        net_endpoint_set_error(
            stream->m_endpoint->m_base_endpoint, net_endpoint_error_source_network,
            net_endpoint_network_errno_remote_closed, net_http2_error_code_str(error_code));
        if (net_endpoint_set_state(stream->m_endpoint->m_base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(stream->m_endpoint->m_base_endpoint, net_endpoint_state_deleting);
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
        protocol->m_em, "http2: %s: %d:    <==       invalid head: %.*s = %.*s",
        net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
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
            protocol->m_em, "http2: %s: %d:    <==       head: %.*s = %.*s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            frame->hd.stream_id, (int)namelen, (const char *)name, (int)valuelen, (const char *)value);
    }

    net_http2_endpoint_stream_t stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (!stream) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: http2: %d: head no bind endpoint",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            frame->hd.stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
        if (net_http2_endpoint_stream_on_head(
                stream,
                (const char *)name, (uint32_t)namelen, (const char *)value, (uint32_t)valuelen)
            != 0)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }
    else if (frame->headers.cat == NGHTTP2_HCAT_HEADERS) {
        if (net_http2_endpoint_stream_on_tailer(
                stream,
                (const char *)name, (uint32_t)namelen, (const char *)value, (uint32_t)valuelen)
            != 0)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }
    else {
        CPE_INFO(
            protocol->m_em, "http2: %s: http2: %d: ignore header cat %d",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
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
        protocol->m_em, "http2: %s: http2: error: code=%d %.*s",
        net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
        lib_error_code, (int)len, msg);
    
    return NGHTTP2_NO_ERROR;
}

int net_http2_endpoint_send_data_callback(
    nghttp2_session *session,
    nghttp2_frame *frame, const uint8_t *framehd, size_t length, nghttp2_data_source *source, void *user_data)
{
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_endpoint_stream_t stream = source->ptr;

    uint32_t sended_sz = 0;

    /* if (bufferevent_write(endpoint->m_bev, framehd, 9) < 0) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "http2: %s: http2: %d: send data: write frame head fail", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
    /*         stream->m_stream_id); */
    /*     return NGHTTP2_ERR_CALLBACK_FAILURE; */
    /* } */
    sended_sz += 9;
    
    if (frame->data.padlen > 0) {
        uint8_t padlen = frame->data.padlen - 1;
        /* if (bufferevent_write(endpoint->m_bev, &padlen, 1) < 0) { */
        /*     CPE_ERROR( */
        /*         protocol->m_em, "http2: %s: http2: %d: send data: write padlen %d fail", */
        /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint-m_base_endpoint), */
        /*         stream->m_stream_id, (padlen)); */
        /*     return NGHTTP2_ERR_CALLBACK_FAILURE; */
        /* } */
        sended_sz++;
    }

    while(length > 0) {
        uint32_t block_sz = 0;
        void * buf = net_endpoint_buf_peak(stream->m_endpoint->m_base_endpoint, net_ep_buf_write, &block_sz);
        if (buf == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: http2: %d: send data: get buf fail",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                stream->m_stream_id);
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }

        if (block_sz > length) {
            block_sz = (uint32_t)length;
        }

        /* if (bufferevent_write(endpoint->m_bev, buf, block_sz) < 0) { */
        /*     CPE_ERROR( */
        /*         protocol->m_em, "http2: %s: http2: %d: send data: write block %d fail", */
        /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
        /*         stream->m_stream_id, block_sz); */
        /*     return NGHTTP2_ERR_CALLBACK_FAILURE; */
        /* } */

        net_endpoint_buf_consume(stream->m_endpoint->m_base_endpoint, net_ep_buf_write, block_sz);
        length -= block_sz;
        sended_sz += block_sz;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: http2:    ==>       (ep) %d data",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)sended_sz);
    }

    return 0;
}

int net_http2_endpoint_http2_session_init(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    /*http2_session*/
    nghttp2_session_callbacks * callbacks = NULL;
    if (nghttp2_session_callbacks_new(&callbacks) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: nghttp2_session_callbacks_new error",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return -1;
    }
    assert(callbacks != NULL);

    nghttp2_session_callbacks_set_send_callback(callbacks, net_http2_endpoint_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, net_http2_endpoint_on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, net_http2_endpoint_on_frame_send_callback);
    nghttp2_session_callbacks_set_on_frame_not_send_callback(callbacks, net_http2_endpoint_on_frame_not_send_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, net_http2_endpoint_on_data_chunk_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, net_http2_endpoint_on_stream_close_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, net_http2_endpoint_on_header_callback);
    nghttp2_session_callbacks_set_on_invalid_header_callback(callbacks, net_http2_endpoint_on_invalid_header_callback);
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, net_http2_endpoint_on_begin_headers_callback);
    nghttp2_session_callbacks_set_error_callback2(callbacks, net_http2_endpoint_on_error_callback);
    nghttp2_session_callbacks_set_send_data_callback(callbacks, net_http2_endpoint_send_data_callback);
    
    if (nghttp2_session_client_new(&endpoint->m_http2_session, callbacks, endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: nghttp2_session_client_new error",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        nghttp2_session_callbacks_del(callbacks);
        return -1;
    }

    nghttp2_session_callbacks_del(callbacks);
    return 0;
}

void net_http2_endpoint_http2_session_fini(net_http2_endpoint_t endpoint) {
    if (endpoint->m_http2_session) {
        nghttp2_session_del(endpoint->m_http2_session);
        endpoint->m_http2_session = NULL;
    }
}

int net_http2_endpoint_http2_session_send_settings(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    nghttp2_settings_entry iv[1] = {
        { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };

    /* client 24 bytes magic string will be sent by nghttp2 library */
    int rv = nghttp2_submit_settings(endpoint->m_http2_session, NGHTTP2_FLAG_NONE, iv, CPE_ARRAY_SIZE(iv));
    if (rv != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: http2: Could not submit SETTINGS: %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            nghttp2_strerror(rv));
        return -1;
    }

    return 0;
}
