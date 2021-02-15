#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_http2_req_i.h"
#include "net_http2_stream_i.h"

void net_http2_req_set_req_state(net_http2_req_t req, net_http2_req_state_t state);

net_http2_req_method_t net_http2_req_method(net_http2_req_t req) {
    return req->m_req_method;
}

net_http2_req_state_t net_http2_req_state(net_http2_req_t req) {
    return req->m_req_state;
}

int net_http2_req_start(net_http2_req_t http_req) {
    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (http_req->m_req_state != net_http2_req_state_init) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req %d: can`t start in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            http_req->m_id, net_http2_req_state_str(http_req->m_req_state));
        return -1;
    }

    if (endpoint->m_state != net_http2_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req %d: start in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            http_req->m_id,
            net_http2_endpoint_state_str(endpoint->m_state));
        return -1;
    }

    net_http2_stream_t stream =
        net_http2_stream_create(
            endpoint,
            nghttp2_session_get_next_stream_id(endpoint->m_http2_session), net_http2_stream_runing_mode_cli);
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req %d: create stream failed",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            http_req->m_id);
        return -1;
    }

    const nghttp2_priority_spec * pri_spec = NULL;

    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, NGHTTP2_FLAG_NONE, -1, pri_spec,
        http_req->m_req_headers, http_req->m_req_head_count, stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id,
            nghttp2_strerror(rv));
        net_http2_stream_free(stream);
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> req %d start failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id);
        net_http2_stream_free(stream);
        return -1;
    }
    
    net_http2_req_set_stream(http_req, stream);

    net_http2_req_set_req_state(http_req, net_http2_req_state_connecting);

    return 0;
}

int net_http2_req_on_req_head_complete(net_http2_req_t req) {
    return 0;
}

void net_http2_req_set_req_state(net_http2_req_t http_req, net_http2_req_state_t state) {
    if (http_req->m_req_state == state) return;

    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: req %d: req-state %s ==> %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            http_req->m_id,
            net_http2_req_state_str(http_req->m_req_state), net_http2_req_state_str(state));
    }

    http_req->m_req_state = state;
}

const char * net_http2_req_state_str(net_http2_req_state_t req_state) {
    switch(req_state) {
    case net_http2_req_state_init:
        return "init";
    case net_http2_req_state_connecting:
        return "connecting";
    case net_http2_req_state_established:
        return "established";
    case net_http2_req_state_read_closed:
        return "read-closed";
    case net_http2_req_state_write_closed:
        return "write-closed";
    case net_http2_req_state_error:
        return "error";
    case net_http2_req_state_done:
        return "done";
    }
}
