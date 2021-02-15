#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_http2_req_i.h"
#include "net_http2_stream_i.h"

void net_http2_req_set_req_state(net_http2_req_t req, net_http2_req_state_t state);

net_http2_req_state_t net_http2_req_state(net_http2_req_t req) {
    return req->m_state;
}

int net_http2_req_add_req_head(net_http2_req_t http_req, const char * attr_name, const char * attr_value) {
    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (http_req->m_req_head_count >= http_req->m_req_head_capacity) {
        uint16_t new_capacity = http_req->m_req_head_capacity < 8 ? 8 : http_req->m_req_head_capacity * 2;
        nghttp2_nv * new_headers = mem_alloc(protocol->m_alloc, sizeof(nghttp2_nv) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: req %d: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                http_req->m_id, new_capacity);
            return -1;
        }

        if (http_req->m_req_headers) {
            memcpy(new_headers, http_req->m_req_headers, sizeof(nghttp2_nv) * http_req->m_req_head_count);
            mem_free(protocol->m_alloc, http_req->m_req_headers);
        }

        http_req->m_req_headers = new_headers;
        http_req->m_req_head_capacity = new_capacity;
    }
    
    nghttp2_nv * nv = http_req->m_req_headers + http_req->m_req_head_count;
    nv->namelen = strlen(attr_name);
    nv->name = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_name, nv->namelen);
    if (nv->name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: add header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), http_req->m_id, attr_name);
        return -1;
    }

    nv->valuelen = strlen(attr_value);
    nv->value = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_value, nv->valuelen);
    if (nv->value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: add header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), http_req->m_id, attr_value);
        mem_free(protocol->m_alloc, nv->name);
        return -1;
    }

    nv->flags = NGHTTP2_NV_FLAG_NO_COPY_VALUE | NGHTTP2_NV_FLAG_NO_COPY_VALUE;

    http_req->m_req_head_count++;
    return 0;
}

const char * net_http2_req_find_req_header(net_http2_req_t req, const char * name) {
    uint16_t i;

    for(i = 0; i < req->m_req_head_count; i++) {
        nghttp2_nv * header = req->m_req_headers + i;
        if (strcmp((const char *)header->name, name) == 0) return (const char *)header->value;
    }
    
    return NULL;
}

int net_http2_req_start(net_http2_req_t http_req) {
    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (http_req->m_state != net_http2_req_state_init) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: can`t start in req-state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            http_req->m_id, net_http2_req_state_str(http_req->m_state));
        return -1;
    }

    if (endpoint->m_state != net_http2_endpoint_state_streaming) {
        net_http2_req_set_req_state(http_req, net_http2_req_state_connecting);
        return 0;
    }

    net_http2_stream_t stream =
        net_http2_stream_create(
            endpoint,
            nghttp2_session_get_next_stream_id(endpoint->m_http2_session), net_http2_stream_runing_mode_cli);
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: create stream failed",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            http_req->m_id);
        return -1;
    }

    const nghttp2_priority_spec * pri_spec = NULL;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    
    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, flags, -1, pri_spec,
        http_req->m_req_headers, http_req->m_req_head_count, stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id, nghttp2_strerror(rv));
        net_http2_stream_free(stream);
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d ==> start failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id);
        net_http2_stream_free(stream);
        return -1;
    }
    
    net_http2_req_set_stream(http_req, stream);

    net_http2_req_set_req_state(http_req, net_http2_req_state_head_sended);

    return 0;
}

int net_http2_req_append(net_http2_req_t http_req, void const * data, uint32_t data_len, uint8_t have_follow_data) {
    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream = http_req->m_stream;

    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: send data no stream",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            http_req->m_id);
        return -1;
    }

    if (http_req->m_state != net_http2_req_state_established
        && http_req->m_state != net_http2_req_state_read_closed)
    {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: can`t send data in req-state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id, net_http2_req_state_str(http_req->m_state));
        return -1;
    }

    if (endpoint->m_state != net_http2_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: send data in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id, net_http2_endpoint_state_str(endpoint->m_state));
        return -1;
    }

    const nghttp2_data_provider * data_prd = NULL;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    if (!have_follow_data) {
        flags |= NGHTTP2_FLAG_END_STREAM;
    }
    
    int rv = nghttp2_submit_data(endpoint->m_http2_session, flags, stream->m_stream_id,  data_prd);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> send %d data fail, error=%s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id, data_len, nghttp2_strerror(rv));
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: ==> req %d start failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, http_req->m_id);
        return -1;
    }

    return 0;
}

int net_http2_req_on_req_head_complete(net_http2_req_t http_req) {
    net_http2_req_set_req_state(http_req, net_http2_req_state_established);
    return 0;
}

void net_http2_req_set_req_state(net_http2_req_t http_req, net_http2_req_state_t state) {
    if (http_req->m_state == state) return;

    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        if (http_req->m_stream) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: http2: %d: req %d: req-state %s ==> %s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                http_req->m_stream->m_stream_id, http_req->m_id,
                net_http2_req_state_str(http_req->m_state), net_http2_req_state_str(state));
        } else {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: req %d: req-state %s ==> %s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                http_req->m_id,
                net_http2_req_state_str(http_req->m_state), net_http2_req_state_str(state));
        }
    }

    http_req->m_state = state;
}

const char * net_http2_req_state_str(net_http2_req_state_t req_state) {
    switch(req_state) {
    case net_http2_req_state_init:
        return "init";
    case net_http2_req_state_connecting:
        return "connecting";
    case net_http2_req_state_head_sended:
        return "head-sended";
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
