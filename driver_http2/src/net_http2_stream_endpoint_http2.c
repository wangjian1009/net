#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_utils.h"

ssize_t net_http2_stream_endpoint_read_callback(
    nghttp2_session *session, int32_t stream_id,
    uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
{
    net_http2_stream_endpoint_t stream = source->ptr;
    net_http2_endpoint_t control = stream->m_control;
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    assert(stream->m_send_processing);

    uint32_t flags = NGHTTP2_DATA_FLAG_NO_COPY;
    
    uint32_t read_len = net_endpoint_buf_size(stream->m_base_endpoint, net_ep_buf_write);
    if (read_len == 0) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    else {
        if (read_len <= length) {
            flags |= NGHTTP2_DATA_FLAG_EOF;
        }
        else {
            read_len = (uint32_t)length;
        }
    }

    *data_flags = flags;

    if (net_endpoint_driver_debug(stream->m_base_endpoint) >= 3) {
        CPE_INFO(
            driver->m_em, "http2: %s: write %d data, eof=%s",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            (int)read_len, (flags & NGHTTP2_DATA_FLAG_EOF) ? "true" : "false");
    }

    if (flags & NGHTTP2_DATA_FLAG_EOF) {
        stream->m_send_processing = 0;
    }
    
    return (ssize_t)read_len;
}

void net_http2_stream_endpoint_schedule_send_data(net_http2_stream_endpoint_t stream) {
    net_http2_endpoint_t control = stream->m_control;
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    assert(net_endpoint_is_writing(stream->m_base_endpoint));
    assert(!stream->m_send_scheduled);
    assert(stream->m_base_endpoint);
    
    stream->m_send_scheduled  = 1;
    TAILQ_INSERT_TAIL(&control->m_sending_streams, stream, m_next_for_sending);

    if (net_endpoint_protocol_debug(stream->m_base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "http2: %s: %s: schedule send begin",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(stream)));
    }

    net_http2_endpoint_schedule_flush(control);
}


void net_http2_stream_endpoint_check_schedule_send_data(net_http2_stream_endpoint_t stream) {
    if (!stream->m_send_scheduled) {
        if (!net_endpoint_buf_is_empty(stream->m_base_endpoint, net_ep_buf_write) /*有数据需要发送 */
            && !net_endpoint_is_writing(stream->m_base_endpoint) /*没有在发送中 */
        ) {
            net_endpoint_set_is_writing(stream->m_base_endpoint, 1);
            net_http2_stream_endpoint_schedule_send_data(stream);
        }
    }
}

int net_http2_stream_endpoint_delay_send_data(net_http2_stream_endpoint_t stream) {
    net_http2_endpoint_t control = stream->m_control;
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    assert(stream);
    assert(stream->m_base_endpoint);
    assert(net_endpoint_is_writing(stream->m_base_endpoint));
    assert(!stream->m_send_processing);

    assert(!net_endpoint_buf_is_empty(stream->m_base_endpoint, net_ep_buf_write));

    assert(stream->m_send_scheduled);
    stream->m_send_scheduled = 0;
    TAILQ_REMOVE(&control->m_sending_streams, stream, m_next_for_sending);

    if (net_endpoint_driver_debug(stream->m_base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "http2: %s: %s: %d: schedule send cleared(in delay send)",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(stream)),
            stream->m_stream_id);
    }
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = stream;
    data_prd.read_callback = net_http2_stream_endpoint_read_callback;

    int rv = nghttp2_submit_data(control->m_http2_session, 0, stream->m_stream_id, &data_prd);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: %d: submit data fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(stream)),
            stream->m_stream_id, nghttp2_strerror(rv));
        return -1;
    }
    stream->m_send_processing = 1;

    return 0;
}

int net_http2_stream_endpoint_sync_state(net_http2_stream_endpoint_t stream) {
    assert(stream->m_control);
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));
    net_endpoint_t control_base = stream->m_control->m_base_endpoint;

    switch(net_endpoint_state(control_base)) {
    case net_endpoint_state_resolving:
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        break;
    case net_endpoint_state_connecting:
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_http2_endpoint_runing_mode(stream->m_control) == net_http2_endpoint_runing_mode_cli) {
            if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_connecting) != 0) return -1;

            if (stream->m_control->m_state == net_http2_endpoint_state_streaming) {
                if (stream->m_control->m_runing_mode == net_http2_endpoint_runing_mode_cli) {
                    if (net_http2_stream_endpoint_send_connect_request(stream) != 0) return -1;
                }
            }
        }
        else {
            assert(net_http2_endpoint_runing_mode(stream->m_control) == net_http2_endpoint_runing_mode_svr);
            if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_established) != 0) return -1;
        }
        break;
    case net_endpoint_state_error:
        if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) {
            net_endpoint_set_error(
                stream->m_base_endpoint,
                net_endpoint_error_source(control_base),
                net_endpoint_error_no(control_base),
                net_endpoint_error_msg(control_base));
                
        }
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) return -1;
        break;
    case net_endpoint_state_read_closed:
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1;
        break;
    case net_endpoint_state_write_closed:
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1;
        break;
    case net_endpoint_state_disable:
        if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_disable) != 0) return -1;
        break;
    case net_endpoint_state_deleting:
        break;
    }

    return 0;
}

int net_http2_stream_endpoint_send_connect_request(net_http2_stream_endpoint_t stream) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));
    net_http2_endpoint_t control = stream->m_control;
    assert(control);

    assert(stream->m_stream_id == -1);
    
    net_address_t target_address = net_endpoint_remote_address(stream->m_base_endpoint);
    if (target_address == NULL) {
        CPE_ERROR(
            driver->m_em, "http2: %s: send connect request: no remote target!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint));
        return -1;
    }

    char target_path[256];
    cpe_str_dup(
        target_path, sizeof(target_path),
        net_address_dump(net_http2_stream_driver_tmp_buffer(driver), target_address));
    
    nghttp2_nv hdrs[] = {
        MAKE_NV2(":method", "CONNECT"),
        MAKE_NV(":authority", target_path, strlen(target_path)),
    };

    const nghttp2_priority_spec * pri_spec = NULL;

    int rv = nghttp2_submit_headers(
        control->m_http2_session, NGHTTP2_FLAG_NONE, -1, pri_spec, hdrs, CPE_ARRAY_SIZE(hdrs), stream);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: http2: %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }
    stream->m_stream_id = (int32_t)rv;

    if (net_endpoint_driver_debug(stream->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "http2: %s: %s: http2: %d: ==> submit headers",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id);
    }

    net_http2_endpoint_schedule_flush(control);

    return 0;
}

int net_http2_stream_endpoint_send_rst_and_schedule(net_http2_stream_endpoint_t stream) {
    assert(stream->m_stream_id != -1);
    assert(stream->m_control);

    net_http2_endpoint_t control = stream->m_control;
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));

    uint32_t error_code = 0;
    int rv = nghttp2_submit_rst_stream(control->m_http2_session, NGHTTP2_FLAG_NONE, stream->m_stream_id, error_code);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: http2: %d: submit rst fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }

    rv = nghttp2_session_set_stream_user_data(control->m_http2_session, stream->m_stream_id, NULL);
    if (rv != 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: http2: %d: clear stream data fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }

    if (net_endpoint_driver_debug(stream->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "http2: %s: %s: http2: %d: ==> rst",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id);
    }
    
    stream->m_stream_id = -1;
    if (stream->m_send_scheduled) {
        stream->m_send_scheduled = 0;
        TAILQ_REMOVE(&control->m_sending_streams, stream, m_next_for_sending);
    }

    net_http2_endpoint_schedule_flush(control);

    return 0;
}

int net_http2_stream_endpoint_send_connect_response(net_http2_stream_endpoint_t stream) {
    net_http2_stream_driver_t driver = net_driver_data(net_endpoint_driver(stream->m_base_endpoint));
    net_http2_endpoint_t control = stream->m_control;
    assert(control);

    nghttp2_nv hdrs[] = {
        MAKE_NV2(":status", "200"),
    };

    nghttp2_data_provider data_prd;
    data_prd.source.ptr = stream;
    data_prd.read_callback = net_http2_stream_endpoint_read_callback;
    
    int rv = nghttp2_submit_response(control->m_http2_session, stream->m_stream_id, hdrs, CPE_ARRAY_SIZE(hdrs), NULL);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: %s: http2: %d: ==> submit response fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }

    if (net_endpoint_driver_debug(stream->m_base_endpoint)) {
        CPE_INFO(
            driver->m_em, "http2: %s: %s: http2: %d: ==> submit response",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(control->m_runing_mode),
            stream->m_stream_id);
    }

    net_http2_endpoint_schedule_flush(control);

    return 0;
}
