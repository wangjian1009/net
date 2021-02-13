#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_http2_stream_i.h"
#include "net_http2_protocol_i.h"
#include "net_http2_utils.h"

int net_http2_stream_commit_headers(net_http2_stream_t stream) {
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    assert(endpoint);

    assert(stream->m_stream_id == -1);
    
    net_address_t target_address = net_endpoint_remote_address(endpoint->m_base_endpoint);
    if (target_address == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: send connect request: no remote target!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return -1;
    }

    char target_path[256];
    cpe_str_dup(
        target_path, sizeof(target_path),
        net_address_dump(net_http2_protocol_tmp_buffer(protocol), target_address));
    
    nghttp2_nv hdrs[] = {
        MAKE_NV2(":method", "CONNECT"),
        MAKE_NV(":authority", target_path, strlen(target_path)),
    };

    const nghttp2_priority_spec * pri_spec = NULL;

    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, NGHTTP2_FLAG_NONE, -1, pri_spec, hdrs, CPE_ARRAY_SIZE(hdrs), stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }
    stream->m_stream_id = (int32_t)rv;

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit headers",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id);
    }

    return 0;
}

ssize_t net_http2_stream_read_callback(
    nghttp2_session *session, int32_t stream_id,
    uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
{
    net_http2_stream_t stream = source->ptr;
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    uint32_t flags = NGHTTP2_DATA_FLAG_NO_COPY;
    
    uint32_t read_len = net_endpoint_buf_size(endpoint->m_base_endpoint, net_ep_buf_write);
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

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 3) {
        CPE_INFO(
            protocol->m_em, "http2: %s: write %d data, eof=%s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)read_len, (flags & NGHTTP2_DATA_FLAG_EOF) ? "true" : "false");
    }

    if (flags & NGHTTP2_DATA_FLAG_EOF) {
        //stream->m_send_processing = 0;
    }
    
    return (ssize_t)read_len;
}

int net_http2_stream_send_data(net_http2_stream_t stream) {
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    assert(endpoint);

    /* if (net_endpoint_driver_debug(stream->m_base_endpoint) >= 2) { */
    /*     CPE_INFO( */
    /*         driver->m_em, "http2: %s: %s: %d: schedule send cleared(in delay send)", */
    /*         net_endpoint_dump(net_http2_stream_driver_tmp_buffer(driver), stream->m_base_endpoint), */
    /*         net_http2_endpoint_runing_mode_str(net_http2_stream_endpoint_runing_mode(stream)), */
    /*         stream->m_stream_id); */
    /* } */
    
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = stream;
    data_prd.read_callback = net_http2_stream_read_callback;

    int rv = nghttp2_submit_data(endpoint->m_http2_session, 0, stream->m_stream_id, &data_prd);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: %d: submit data fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, nghttp2_strerror(rv));
        return -1;
    }

    return 0;
}
