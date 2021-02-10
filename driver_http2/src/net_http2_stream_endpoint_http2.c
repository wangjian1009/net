#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_endpoint_i.h"

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
            driver->m_em, "http2: %s: schedule send cleared(in delay send)",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint));
    }
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = stream;
    data_prd.read_callback = net_http2_stream_endpoint_read_callback;

    int rv = nghttp2_submit_data(control->m_http2_session, 0, stream->m_stream_id, &data_prd);
    if (rv < 0) {
        CPE_ERROR(
            driver->m_em, "http2: %s: submit data fail, %s!",
            net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint),
            nghttp2_strerror(rv));
        return -1;
    }
    stream->m_send_processing = 1;

    return 0;
}

int net_http2_stream_endpoint_sync_state(net_http2_stream_endpoint_t stream) {
    assert(stream->m_control);
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
                if (stream->m_state == net_http2_stream_endpoint_state_init) {
                    if (net_http2_stream_endpoint_send_connect_request(stream) != 0) return -1;
                    if (net_http2_stream_endpoint_set_state(stream, net_http2_stream_endpoint_state_connecting) != 0) return -1;
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
