#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_stream_i.h"
#include "net_http2_processor_i.h"
#include "net_http2_req_i.h"
#include "net_http2_utils.h"

ssize_t net_http2_endpoint_send_callback(
    nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

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
        char name_buf[128];
        cpe_str_dup(
            name_buf, sizeof(name_buf),
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));

        CPE_INFO(
            protocol->m_em, "http2: %s: %s: net: ==> net %d bytes\n%s",
            name_buf, net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), (int)length,
            mem_buffer_dump_data(net_http2_protocol_tmp_buffer(protocol), data, length, 0));
    }
    else if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
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

    switch(frame->hd.type) {
    case NGHTTP2_SETTINGS:
        if (frame->hd.flags & NGHTTP2_FLAG_ACK) {
            net_http2_endpoint_set_state(endpoint, net_http2_endpoint_state_streaming);
        }
        break;
    case NGHTTP2_HEADERS:
        if (frame->hd.flags & NGHTTP2_FLAG_END_HEADERS) {
            CPE_ERROR(protocol->m_em, "xxxx 00");
            net_http2_stream_t stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (stream) {
                CPE_ERROR(protocol->m_em, "xxxx 1");
                switch(stream->m_runing_mode) {
                case net_http2_stream_runing_mode_cli: {
                    net_http2_req_t req = net_http2_stream_req(stream);
                    if (req == NULL) {
                        CPE_ERROR(
                            protocol->m_em, "http2: %s: %s: http2: %d: <== recv head, stream no bind request",
                            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                            frame->hd.stream_id);
                        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                    }
                    else if (net_http2_req_on_req_head_complete(req) != 0) {
                        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                    }
                    break;
                }
                case net_http2_stream_runing_mode_svr: {
                    CPE_ERROR(protocol->m_em, "xxxx 1");
                    net_http2_processor_t processor = net_http2_stream_processor(stream);
                    if (processor == NULL) {
                        CPE_ERROR(
                            protocol->m_em, "http2: %s: %s: http2: %d: <== recv head, stream no bind processor",
                            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                            frame->hd.stream_id);
                        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                    }
                    else if (net_http2_processor_on_head_complete(processor) != 0) {
                        CPE_ERROR(protocol->m_em, "xxxx 2");
                        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                    }
                    CPE_ERROR(protocol->m_em, "xxxx 3");
                    break;
                }
                }
            }
        }
        break;
    default:
        break;
    }
    
    return NGHTTP2_NO_ERROR;
}

int net_http2_endpoint_on_frame_schedule_next_send(
    nghttp2_session * session, const nghttp2_frame * frame, net_http2_endpoint_t endpoint)
{
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream;
    
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

            //TODO:
            /* if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) { */
            /*     net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
            /* } */
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

    //TODO:
    /* if (net_endpoint_state(stream->m_base_endpoint) == net_endpoint_state_established) { */
    /*     assert(net_endpoint_is_writing(stream->m_base_endpoint)); */
    /*     assert(!stream->m_send_scheduled); */

    /*     if (net_endpoint_buf_is_empty(stream->m_base_endpoint, net_ep_buf_write)) { */
    /*         net_endpoint_set_is_writing(stream->m_base_endpoint, 0); */
    /*     } */
    /*     else { */
    /*         net_http2_stream_schedule_send_data(stream); */
    /*     } */
    /* } */

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
    net_http2_stream_t stream;

    stream = nghttp2_session_get_stream_user_data(session, stream_id);
    if (!stream) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: recv chunk: no bind endpoint",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    if (net_http2_stream_on_input(stream, data, len) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: recv chunk: input failed",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream_id);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }

    CPE_ERROR(protocol->m_em, "http2: 111");
    
    return NGHTTP2_NO_ERROR;
}

int net_http2_endpoint_on_stream_close_callback(
    nghttp2_session * session, int32_t stream_id, uint32_t error_code, void * user_data) {
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    net_http2_stream_t stream = nghttp2_session_get_stream_user_data(session, stream_id);
    if (stream == NULL) {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: http2: %d: already closed!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                stream_id);
        }
    }
    else {
        net_http2_stream_on_close(stream, error_code);
    }

    return NGHTTP2_NO_ERROR;
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

    return NGHTTP2_NO_ERROR;
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

    net_http2_stream_t stream = NULL;

    if (frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
        stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
        if (stream == NULL) {
            stream = net_http2_stream_create(endpoint, frame->hd.stream_id, net_http2_stream_runing_mode_svr);
            if (stream == NULL) {
                CPE_ERROR(
                    protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header create stream fail",
                    net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                    net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                    frame->hd.stream_id);
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }

            int rv = nghttp2_session_set_stream_user_data(endpoint->m_http2_session, frame->hd.stream_id, stream);
            if (rv != NGHTTP2_NO_ERROR) {
                CPE_ERROR(
                    protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header: bind stream fail, error=%s",
                    net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                    net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                    frame->hd.stream_id, net_http2_error_code_str(rv));
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
        }
    }
    else {
        stream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
        if (!stream) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: <== receive head no bind endpoint",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                frame->hd.stream_id);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: <== %.*s = %.*s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id, (int)namelen, (const char *)name, (int)valuelen, (const char *)value);
    }

    switch(frame->headers.cat) {
    case NGHTTP2_HCAT_REQUEST: {
        net_http2_processor_t processor = net_http2_stream_processor(stream);
        if (processor == NULL) {
            processor = net_http2_processor_create(endpoint);
            if (processor == NULL) {
                CPE_ERROR(
                    protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header create processor fail",
                    net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                    net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                    frame->hd.stream_id);
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }

            net_http2_processor_set_stream(processor, stream);
        }

        if (net_http2_processor_add_head(
                processor, (const char *)name, namelen, (const char *)value, valuelen) != 0)
        {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: <== receive request header add head fail",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                frame->hd.stream_id);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }

        break;
    }
    case NGHTTP2_HCAT_RESPONSE: {
        net_http2_req_t req = net_http2_stream_req(stream);
        if (req == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: <== receive reason header, no bind req",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                frame->hd.stream_id);
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }

        if (cpe_str_cmp_part((const char *)name, namelen, ":status") == 0) {
            char value_buf[64];
            if (valuelen + 1 > CPE_ARRAY_SIZE(value_buf)) {
                CPE_ERROR(
                    protocol->m_em, "http2: %s: %s: http2: %d: on head: status overflow!",
                    net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                    net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                    stream->m_stream_id);
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }

            const char * str_status = cpe_str_dup_len(value_buf, sizeof(value_buf), (const char *)value, valuelen);
            req->m_res_code = atoi(str_status);
        }
        else {
            if (net_http2_req_add_res_head(req, (const char *)name, namelen, (const char *)value, valuelen) != 0) {
                return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            }
        }
        break;
    }
    case NGHTTP2_HCAT_PUSH_RESPONSE:
        break;
    case NGHTTP2_HCAT_HEADERS:
        if (net_http2_stream_on_tailer(
                stream,
                (const char *)name, (uint32_t)namelen, (const char *)value, (uint32_t)valuelen)
            != 0)
        {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
        break;
    default:
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: <== ignore header cat %d",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            frame->hd.stream_id, frame->headers.cat);
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }
    
    return NGHTTP2_NO_ERROR;
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
    
    return NGHTTP2_NO_ERROR;
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
    net_http2_stream_t stream = source->ptr;

    CPE_ERROR(protocol->m_em, "http2: send data 222");
    
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

    //TODO:
    /* if (net_endpoint_buf_append_from_other( */
    /*         endpoint->m_base_endpoint, net_ep_buf_write, */
    /*         stream->m_base_endpoint, net_ep_buf_write, */
    /*         length) != 0) */
    /* { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "http2: %s: %s: http2: %d: ==> write body %d data fail", */
    /*         net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint), */
    /*         net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), */
    /*         stream->m_stream_id, (int)length); */
    /*     return NGHTTP2_ERR_CALLBACK_FAILURE; */
    /* } */

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> %d data",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, (int)head_sz + (int)length);
    }

    CPE_ERROR(protocol->m_em, "http2: send data 333");
    return 0;
}

int net_http2_endpoint_http2_flush(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    int rv = nghttp2_session_send(endpoint->m_http2_session);
    if (rv != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: ==> flush failed: %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            nghttp2_strerror(rv));
        if (net_endpoint_error_source(endpoint->m_base_endpoint) == net_endpoint_error_source_none) {
            net_endpoint_set_error(
                endpoint->m_base_endpoint,
                net_endpoint_error_source_network, net_endpoint_network_errno_logic, nghttp2_strerror(rv));
        }
        if (net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_error) != 0) {
            net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_deleting);
        }
        return -1;
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
