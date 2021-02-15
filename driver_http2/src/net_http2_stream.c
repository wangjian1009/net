#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_i.h"
#include "net_http2_req_i.h"
#include "net_http2_processor_i.h"

net_http2_stream_t
net_http2_stream_create(
    net_http2_endpoint_t endpoint, uint32_t stream_id, net_http2_stream_runing_mode_t runing_mode)
{
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    net_http2_stream_t stream = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_stream));
    stream->m_endpoint = endpoint;
    stream->m_stream_id = stream_id;
    stream->m_read_closed = 0;
    stream->m_write_closed = 0;
    stream->m_runing_mode = runing_mode;

    switch(stream->m_runing_mode) {
    case net_http2_stream_runing_mode_cli:
        stream->m_cli.m_req = NULL;
        break;
    case net_http2_stream_runing_mode_svr:
        stream->m_svr.m_processor = NULL;
        break;
    }
    
    TAILQ_INSERT_TAIL(&endpoint->m_streams, stream, m_next_for_ep);
    
    return stream;
}

void net_http2_stream_free(net_http2_stream_t stream) {
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    switch(stream->m_runing_mode) {
    case net_http2_stream_runing_mode_cli:
        if (stream->m_cli.m_req) {
            net_http2_req_set_stream(stream->m_cli.m_req, NULL);
            assert(stream->m_cli.m_req == NULL);
        }
        break;
    case net_http2_stream_runing_mode_svr:
        break;
    }

    TAILQ_REMOVE(&endpoint->m_streams, stream, m_next_for_ep);
    mem_free(protocol->m_alloc, stream);
}

uint32_t net_http2_stream_id(net_http2_stream_t stream) {
    return stream->m_stream_id;
}

net_http2_stream_runing_mode_t net_http2_stream_runing_mode(net_http2_stream_t stream) {
    return stream->m_runing_mode;
}

net_http2_endpoint_t net_http2_stream_endpoint(net_http2_stream_t stream) {
    return stream->m_endpoint;
}

uint8_t net_http2_stream_read_closed(net_http2_stream_t stream) {
    return stream->m_read_closed;
}

uint8_t net_http2_stream_write_closed(net_http2_stream_t stream) {
    return stream->m_write_closed;
}

net_http2_req_t net_http2_stream_req(net_http2_stream_t stream) {
    return stream->m_runing_mode == net_http2_stream_runing_mode_cli
        ? stream->m_cli.m_req
        : NULL;
}

net_http2_processor_t net_http2_stream_processor(net_http2_stream_t stream) {
    return stream->m_runing_mode == net_http2_stream_runing_mode_svr
        ? stream->m_svr.m_processor
        : NULL;
}    

static ssize_t net_http2_stream_read_callback(
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

int net_http2_stream_send(
    net_http2_stream_t stream, void const * data, uint32_t data_size, uint8_t have_follow_data)
{
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    assert(endpoint);

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: %d: schedule send cleared(in delay send)",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id);
    }
    
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

const char *
net_http2_stream_runing_mode_str(net_http2_stream_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_http2_stream_runing_mode_cli:
        return "cli";
    case net_http2_stream_runing_mode_svr:
        return "svr";
    }
}
