#include "cpe/utils/stream.h"
#include "cpe/utils/stream_buffer.h"
#include "net_http2_utils.h"

void net_http2_print_header(write_stream_t ws, uint8_t prefix_len, nghttp2_nv * nvs, uint8_t nv_count) {
    uint8_t i;
    for (i = 0; i < nv_count; ++i) {
        stream_putc_count(ws, ' ', prefix_len);
        stream_write(ws, nvs[i].name, nvs[i].namelen);
        stream_printf(ws, ": ");
        stream_write(ws, nvs[i].value, nvs[i].valuelen);
    }
}

const char * net_http2_dump_header(mem_buffer_t buffer, nghttp2_nv * nvs, uint8_t nv_count) {
    mem_buffer_clear_data(buffer);
    
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    net_http2_print_header((write_stream_t)&ws, 4, nvs, nv_count);
    stream_putc((write_stream_t)&ws, 0);
    return mem_buffer_make_continuous(buffer, 0);
}

void net_http2_print_frame_priority_spec(write_stream_t ws, nghttp2_priority_spec pri_spec) {
    /* int32_t stream_id; */
    /* int32_t weight; */
    /* uint8_t exclusive; */
}

void net_http2_print_frame_data(write_stream_t ws, const nghttp2_data *frame) {
    stream_printf(ws, ", padlen=%d", frame->padlen);
}

void net_http2_print_frame_headers(write_stream_t ws, const nghttp2_headers *frame) {
    stream_printf(
        ws, ", category=%s",
        net_nghttp2_headers_category_str(frame->cat));

    net_http2_print_frame_priority_spec(ws, frame->pri_spec);

    stream_printf(ws, ", padlen=%d", (int)frame->padlen);

    uint32_t i;
    for (i = 0; i < frame->nvlen; ++i) {
        nghttp2_nv * nv = frame->nva + i;
        stream_printf(
            ws, "\n    %.*s=%.*s",
            (int)nv->namelen, nv->name, (int)nv->valuelen, nv->value);
    }
}

void net_http2_print_frame_goaway(write_stream_t ws, const nghttp2_goaway *frame) {
    stream_printf(
        ws, ", last-stream=%d, error=%s",
        frame->last_stream_id,
        net_http2_error_code_str(frame->error_code));

    stream_printf(ws, "\n");

    stream_dump_data(ws, frame->opaque_data, frame->opaque_data_len, 1);
}

void net_http2_print_frame_settings(write_stream_t ws, const nghttp2_settings *frame) {
    uint32_t i = 0;
    for(i = 0; i < frame->niv; ++i) {
        nghttp2_settings_entry * iv = frame->iv + i;
        stream_printf(
            ws, ", %s=%d",
            net_http2_settings_id_str(iv->settings_id),
            iv->value);
    }
}

void net_http2_print_frame_rst_stream(write_stream_t ws, const nghttp2_rst_stream *frame) {
    stream_printf(ws, ", error=%s", net_http2_error_code_str(frame->error_code));
}

void net_http2_print_frame_window_update(write_stream_t ws, const nghttp2_window_update *frame) {
    stream_printf(ws, ", increment=%d", frame->window_size_increment);
}

void net_http2_print_frame(write_stream_t ws, const nghttp2_frame *frame) {
    uint32_t i;

    stream_printf(
        ws, "type=%s, length=%d, stream=%d",
        net_http2_frame_type_str(frame->hd.type),
        (int)frame->hd.length,
        frame->hd.stream_id);

    nghttp2_flag check_flags[] = {
        NGHTTP2_FLAG_END_HEADERS,
        NGHTTP2_FLAG_ACK,
        NGHTTP2_FLAG_PADDED,
        NGHTTP2_FLAG_PRIORITY
    };

    uint8_t flag_count = 0;
    for(i = 0; i < CPE_ARRAY_SIZE(check_flags); ++i) {
        if (!(frame->hd.flags & check_flags[i])) continue;
        flag_count++;

        if (flag_count == 1) {
            stream_printf(ws, ", flags=[");
        }
        else {
            stream_printf(ws, ",");
        }

        const char * flag_name = net_nghttp2_flag_str(check_flags[i], frame->hd.stream_id == 0 ? 0 : 1);
        stream_printf(ws, "%s", flag_name);
    }

    if (flag_count) {
        stream_printf(ws, "]");
    }

    switch(frame->hd.type) {
    case NGHTTP2_DATA:
        net_http2_print_frame_data(ws, (const nghttp2_data *)frame);
        break;
    case NGHTTP2_HEADERS:
        net_http2_print_frame_headers(ws, (const nghttp2_headers *)frame);
        break;
    case NGHTTP2_GOAWAY:
        net_http2_print_frame_goaway(ws, (const nghttp2_goaway *)frame);
        break;
    case NGHTTP2_SETTINGS:
        net_http2_print_frame_settings(ws, (const nghttp2_settings *)frame);
        break;
    case NGHTTP2_RST_STREAM:
        net_http2_print_frame_rst_stream(ws, (const nghttp2_rst_stream *)frame);
        break;
    case NGHTTP2_WINDOW_UPDATE:
        net_http2_print_frame_window_update(ws, (const nghttp2_window_update *)frame);
        break;
    default:
        break;
    }
}

const char * net_http2_frame_type_str(nghttp2_frame_type frame_type) {
    switch(frame_type) {
    case NGHTTP2_DATA:
        return "data";
    case NGHTTP2_HEADERS:
        return "headers";
    case NGHTTP2_PRIORITY:
        return "priority";
    case NGHTTP2_RST_STREAM:
        return "rst-stream";
    case NGHTTP2_SETTINGS:
        return "settings";
    case NGHTTP2_PUSH_PROMISE:
        return "push-promise";
    case NGHTTP2_PING:
        return "ping";
    case NGHTTP2_GOAWAY:
        return "goaway";
    case NGHTTP2_WINDOW_UPDATE:
        return "window-update";
    case NGHTTP2_CONTINUATION:
        return "continuation";
    case NGHTTP2_ALTSVC:
        return "altsvc";
    case NGHTTP2_ORIGIN:
        return "origin";
    }
}

const char * net_http2_error_code_str(nghttp2_error_code error_code) {
    switch (error_code) {
    case NGHTTP2_NO_ERROR:
        return "none";
    case NGHTTP2_PROTOCOL_ERROR:
        return "protocol-error";
    case NGHTTP2_INTERNAL_ERROR:
        return "internal-error";
    case NGHTTP2_FLOW_CONTROL_ERROR:
        return "flow-control-error";
    case NGHTTP2_SETTINGS_TIMEOUT:
        return "settings-timeout";
    case NGHTTP2_STREAM_CLOSED:
        return "stream-closed";
    case NGHTTP2_FRAME_SIZE_ERROR:
        return "frame-size-error";
    case NGHTTP2_REFUSED_STREAM:
        return "refused-stream";
    case NGHTTP2_CANCEL:
        return "cancel";
    case NGHTTP2_COMPRESSION_ERROR:
        return "compression-error";
    case NGHTTP2_CONNECT_ERROR:
        return "connect-error";
    case NGHTTP2_ENHANCE_YOUR_CALM:
        return "enhance-your-calm";
    case NGHTTP2_INADEQUATE_SECURITY:
        return "inadequate-security";
    case NGHTTP2_HTTP_1_1_REQUIRED:
        return "http_1_1-required";
    }
}

const char * net_http2_settings_id_str(nghttp2_settings_id settings_id) {
    switch(settings_id) {
    case NGHTTP2_SETTINGS_HEADER_TABLE_SIZE:
        return "header-table-size";
    case NGHTTP2_SETTINGS_ENABLE_PUSH:
        return "enable-push";
    case NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
        return "max-concurrent-streams";
    case NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
        return "initial-window-size";
    case NGHTTP2_SETTINGS_MAX_FRAME_SIZE:
        return "max-frame-size";
    case NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
        return "max-header-list-size";
    case NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL:
        return "enable-connect-protocol";
    }
}

const char * net_nghttp2_headers_category_str(nghttp2_headers_category headers_category) {
    switch (headers_category) {
    case NGHTTP2_HCAT_REQUEST:
        return "request";
    case NGHTTP2_HCAT_RESPONSE:
        return "response";
    case NGHTTP2_HCAT_PUSH_RESPONSE:
        return "push-response";
    case NGHTTP2_HCAT_HEADERS:
        return "headers";
    }
}

const char * net_nghttp2_flag_str(nghttp2_flag flag, uint8_t is_for_stream) {
    /*NGHTTP2_FLAG_ACK*/
    switch(flag) {
    case NGHTTP2_FLAG_NONE:
        return "none";
    case NGHTTP2_FLAG_END_STREAM:
        return is_for_stream ? "end-stream" : "ack";
    case NGHTTP2_FLAG_END_HEADERS:
        return "end-headers";
    case NGHTTP2_FLAG_PADDED:
        return "padded";
    case NGHTTP2_FLAG_PRIORITY:
        return "priority";
    }
}

const char * net_nghttp2_stream_state_str(nghttp2_stream_proto_state state) {
    switch(state) {
    case NGHTTP2_STREAM_STATE_IDLE:
        return "resolved_idle";
    case NGHTTP2_STREAM_STATE_OPEN:
        return "resolved_open";
    case NGHTTP2_STREAM_STATE_RESERVED_LOCAL:
        return "resolved_local";
    case NGHTTP2_STREAM_STATE_RESERVED_REMOTE:
        return "resolved_remote";
    case NGHTTP2_STREAM_STATE_HALF_CLOSED_LOCAL:
        return "half_closed_local";
    case NGHTTP2_STREAM_STATE_HALF_CLOSED_REMOTE:
        return "half_closed_remote";
    case NGHTTP2_STREAM_STATE_CLOSED:
        return "closed";
    }
}
