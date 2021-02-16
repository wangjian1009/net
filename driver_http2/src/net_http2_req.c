#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http2_req_i.h"
#include "net_http2_stream_i.h"

net_http2_req_t
net_http2_req_create(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    net_http2_req_t req = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_req));
    if (req == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req: create: alloc fail!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode));
        return NULL;
    }

    req->m_endpoint = endpoint;
    req->m_stream = NULL;
    req->m_id = ++protocol->m_max_req_id;
    req->m_state = net_http2_req_state_init;

    req->m_req_head_count = 0;
    req->m_req_head_capacity = 0;
    req->m_req_headers = NULL;
    
    req->m_res_head_count = 0;
    req->m_res_head_capacity = 0;
    req->m_res_headers = NULL;

    req->m_read_ctx = NULL;
    req->m_read_ctx_free = NULL;
    req->m_on_state_change = NULL;
    req->m_on_recv = NULL;

    req->m_write_ctx = NULL;
    req->m_write_ctx_free = NULL;
    req->m_on_write = NULL;
    req->m_is_write_started = 0;
    req->m_have_follow_data = 0;
    mem_buffer_init(&req->m_write_data_buffer, protocol->m_alloc);

    endpoint->m_req_count++;
    TAILQ_INSERT_TAIL(&endpoint->m_reqs, req, m_next_for_endpoint);

    return req;
}

void net_http2_req_free(net_http2_req_t req) {
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (req->m_write_ctx_free) {
        req->m_write_ctx_free(req->m_write_ctx);
        req->m_write_ctx_free = NULL;
    }

    if (req->m_read_ctx_free) {
        req->m_read_ctx_free(req->m_read_ctx);
        req->m_read_ctx_free = NULL;
    }
    
    /*req*/
    uint16_t i;
    for(i = 0; i < req->m_req_head_count; ++i) {
        mem_free(protocol->m_alloc, req->m_req_headers[i].name);
        mem_free(protocol->m_alloc, req->m_req_headers[i].value);
    }
    req->m_req_head_count = 0;
    
    if (req->m_req_headers) {
        mem_free(protocol->m_alloc, req->m_req_headers);
        req->m_req_headers = NULL;
        req->m_req_head_capacity = 0;
    }
            
    /*res*/
    for(i = 0; i < req->m_res_head_count; ++i) {
        mem_free(protocol->m_alloc, req->m_res_headers[i].name);
        mem_free(protocol->m_alloc, req->m_res_headers[i].value);
    }
    req->m_res_head_count = 0;
    
    if (req->m_res_headers) {
        mem_free(protocol->m_alloc, req->m_res_headers);
        req->m_res_headers = NULL;
        req->m_res_head_capacity = 0;
    }

    /*stream*/
    if (req->m_stream) {
        net_http2_req_set_stream(req, NULL);
        assert(req->m_stream == NULL);
    }

    mem_buffer_clear(&req->m_write_data_buffer);

    assert(http_ep->m_req_count > 0);
    http_ep->m_req_count--;
    TAILQ_REMOVE(&http_ep->m_reqs, req, m_next_for_endpoint);
    
    mem_free(protocol->m_alloc, req);
}

net_http2_req_t
net_http2_req_find(net_http2_endpoint_t http_ep, uint16_t req_id) {
    net_http2_req_t req;

    TAILQ_FOREACH(req, &http_ep->m_reqs, m_next_for_endpoint) {
        if (req->m_id == req_id) {
            return req;
        }
    }

    return NULL;
}

uint16_t net_http2_req_id(net_http2_req_t req) {
    return req->m_id;
}

net_http2_endpoint_t net_http2_req_endpoint(net_http2_req_t req) {
    return req->m_endpoint;
}

net_http2_stream_t net_http2_req_stream(net_http2_req_t req) {
    return req->m_stream;
}

net_http2_req_state_t net_http2_req_state(net_http2_req_t req) {
    return req->m_state;
}

int net_http2_req_add_req_head2(
    net_http2_req_t req, const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (req->m_req_head_count >= req->m_req_head_capacity) {
        uint16_t new_capacity = req->m_req_head_capacity < 8 ? 8 : req->m_req_head_capacity * 2;
        nghttp2_nv * new_headers = mem_alloc(protocol->m_alloc, sizeof(nghttp2_nv) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: req %d: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                req->m_id, new_capacity);
            return -1;
        }

        if (req->m_req_headers) {
            memcpy(new_headers, req->m_req_headers, sizeof(nghttp2_nv) * req->m_req_head_count);
            mem_free(protocol->m_alloc, req->m_req_headers);
        }

        req->m_req_headers = new_headers;
        req->m_req_head_capacity = new_capacity;
    }
    
    nghttp2_nv * nv = req->m_req_headers + req->m_req_head_count;
    nv->namelen = name_len;
    nv->name = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_name, nv->namelen);
    if (nv->name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: add header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), req->m_id, attr_name);
        return -1;
    }

    nv->valuelen = value_len;
    nv->value = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_value, nv->valuelen);
    if (nv->value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: add header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode), req->m_id, attr_value);
        mem_free(protocol->m_alloc, nv->name);
        return -1;
    }

    nv->flags = NGHTTP2_NV_FLAG_NO_COPY_VALUE | NGHTTP2_NV_FLAG_NO_COPY_VALUE;

    req->m_req_head_count++;
    return 0;
}

int net_http2_req_add_req_head(net_http2_req_t req, const char * attr_name, const char * attr_value) {
    return net_http2_req_add_req_head2(req, attr_name, strlen(attr_name), attr_value, strlen(attr_value));
}

const char * net_http2_req_find_req_header(net_http2_req_t req, const char * name) {
    uint16_t i;

    for(i = 0; i < req->m_req_head_count; i++) {
        nghttp2_nv * header = req->m_req_headers + i;
        if (strcmp((const char *)header->name, name) == 0) return (const char *)header->value;
    }
    
    return NULL;
}

int net_http2_req_add_res_head2(
    net_http2_req_t req,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    net_http2_stream_t stream = req->m_stream;
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: add res header: no stream!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }

    if (req->m_res_head_count >= req->m_res_head_capacity) {
        uint16_t new_capacity = req->m_res_head_capacity < 8 ? 8 : req->m_res_head_capacity * 2;
        nghttp2_nv * new_headers =
            mem_alloc(protocol->m_alloc, sizeof(nghttp2_nv) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: http2: %d: req: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                stream->m_stream_id, new_capacity);
            return -1;
        }

        if (req->m_res_headers) {
            memcpy(new_headers, req->m_res_headers, sizeof(nghttp2_nv) * req->m_res_head_count);
            mem_free(protocol->m_alloc, req->m_res_headers);
        }

        req->m_res_headers = new_headers;
        req->m_res_head_capacity = new_capacity;
    }
    
    nghttp2_nv * nv = req->m_res_headers + req->m_res_head_count;

    nv->namelen = name_len;
    nv->name = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_name, nv->namelen);
    if (nv->name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: add res header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, attr_name);
        return -1;
    }

    nv->valuelen = value_len;
    nv->value = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_value, nv->valuelen);
    if (nv->value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: add res header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, attr_value);
        mem_free(protocol->m_alloc, nv->name);
        return -1;
    }

    req->m_res_head_count++;
    return 0;
}

int net_http2_req_add_res_head(net_http2_req_t req, const char * attr_name, const char * attr_value) {
    return net_http2_req_add_res_head2(req, attr_name, strlen(attr_name), attr_value, strlen(attr_value));
}

const char * net_http2_req_find_res_header(net_http2_req_t req, const char * name) {
    uint16_t i;

    for(i = 0; i < req->m_res_head_count; i++) {
        nghttp2_nv * header = req->m_res_headers + i;
        if (strcmp((const char *)header->name, name) == 0) return (const char *)header->value;
    }

    return NULL;
}

int net_http2_req_start_request(net_http2_req_t req, uint8_t have_follow_data) {
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (req->m_state != net_http2_req_state_init) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: start-request: can`t start in req-state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id, net_http2_req_state_str(req->m_state));
        return -1;
    }

    if (endpoint->m_state != net_http2_endpoint_state_streaming) {
        net_http2_req_set_req_state(req, net_http2_req_state_connecting);
        return 0;
    }

    net_http2_stream_t stream =
        net_http2_stream_create(
            endpoint,
            nghttp2_session_get_next_stream_id(endpoint->m_http2_session), net_http2_stream_runing_mode_cli);
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: start-request: create stream failed",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }

    const nghttp2_priority_spec * pri_spec = NULL;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    if (!have_follow_data) {
        flags |= NGHTTP2_FLAG_END_STREAM;
    }
    
    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, flags, -1, pri_spec,
        req->m_req_headers, req->m_req_head_count, stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> submit request fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, nghttp2_strerror(rv));
        net_http2_stream_free_no_unbind(stream);
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d ==> start request failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id);
        net_http2_stream_free(stream);
        return -1;
    }

    req->m_have_follow_data = have_follow_data;
    net_http2_req_set_stream(req, stream);

    net_http2_req_set_req_state(req, net_http2_req_state_head_sended);

    return 0;
}

int net_http2_req_start_response(net_http2_req_t req, uint8_t have_follow_data) {
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (req->m_state != net_http2_req_state_head_received) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: start-response: can`t start in req-state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id, net_http2_req_state_str(req->m_state));
        return -1;
    }

    net_http2_stream_t stream = req->m_stream;
    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: start-response: no stream",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }

    const nghttp2_data_provider * data_prd = NULL;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    if (!have_follow_data) {
        flags |= NGHTTP2_FLAG_END_STREAM;
    }
    
    int rv = nghttp2_submit_headers(
        endpoint->m_http2_session, flags, stream->m_stream_id, NULL,
        req->m_res_headers, req->m_res_head_count, stream);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> submit response fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, nghttp2_strerror(rv));
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d ==> flush response failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id);
        return -1;
    }

    req->m_have_follow_data = have_follow_data;

    net_http2_req_set_req_state(req, net_http2_req_state_established);

    return 0;
}

uint8_t net_http2_req_on_write_from_buf(void * ctx, net_http2_req_t req, void * output, uint32_t * output_len) {
    size_t sz = mem_buffer_read(output, *output_len, &req->m_write_data_buffer);
    mem_buffer_remove(&req->m_write_data_buffer, sz);
    *output_len = sz;
    return mem_buffer_size(&req->m_write_data_buffer) == 0 ? 0 : 1;
}

int net_http2_req_append(net_http2_req_t req, void const * data, uint32_t data_len, uint8_t have_follow_data) {
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream = req->m_stream;

    if (!req->m_have_follow_data) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: append: no follow data, can`t append!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }
    
    if (req->m_on_write != NULL
        && req->m_on_write != net_http2_req_on_write_from_buf) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: append: already have writer!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }

    if (mem_buffer_append(&req->m_write_data_buffer, data, data_len) < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: append: append data to buf fail!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }

    return req->m_on_write == NULL
        ? net_http2_req_write(req, req, net_http2_req_on_write_from_buf, NULL, have_follow_data)
        : 0;
}

ssize_t net_http2_req_do_write(
    nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
    uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
{
    net_http2_req_t req = source->ptr;
    net_http2_endpoint_t endpoint = user_data;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream = req->m_stream;

    assert(stream);
    assert(stream->m_stream_id == stream_id);

    uint32_t read_len = 0;
    uint8_t have_continue_data = 0;

    assert(req->m_is_write_started);

    if (req->m_on_write == NULL) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    else {
        //*data_flags |= NGHTTP2_DATA_FLAG_NO_COPY;
        read_len = length;
        have_continue_data = req->m_on_write(req->m_write_ctx, req, buf, &read_len);

        if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 3) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: http2: %d: write %d data, eof=%s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                stream_id, read_len, have_continue_data ? "true" : "false");
        }
    }

    if (!have_continue_data) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        if (req->m_write_ctx_free) {
            req->m_write_ctx_free(req->m_write_ctx);
        }
        req->m_write_ctx = NULL;
        req->m_write_ctx_free = NULL;
        req->m_on_write = NULL;
        req->m_is_write_started = 0;
    }
    
    return (ssize_t)read_len;
}

int net_http2_req_begin_write(net_http2_req_t req) {
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream = req->m_stream;
    assert(stream);

    assert(req->m_on_write != NULL);
    assert(req->m_is_write_started == 0);

    nghttp2_data_provider data_prd;
    data_prd.source.ptr = req;
    data_prd.read_callback = net_http2_req_do_write;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    if (!req->m_have_follow_data) {
        flags |= NGHTTP2_FLAG_END_STREAM;
    }
    
    req->m_is_write_started = 1;
    int rv = nghttp2_submit_data(endpoint->m_http2_session, flags, stream->m_stream_id, &data_prd);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> start write fail, error=%s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, nghttp2_strerror(rv));
        req->m_is_write_started = 0;
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: ==> flush failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id);
        return -1;
    }
    
    return 0;
}

int net_http2_req_write(
    net_http2_req_t req,
    void * write_ctx, net_http2_req_on_write_fun_t on_write, void (*write_ctx_free)(void *),
    uint8_t have_follow_data)
{
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    net_http2_stream_t stream = req->m_stream;

    if (stream == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: write: no stream",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            req->m_id);
        return -1;
    }
    
    if (!req->m_have_follow_data) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: write: no follow data, can`t append!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id);
        return -1;
    }
    
    if (req->m_on_write != NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: write: already have writer!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id);
        return -1;
    }

    switch(req->m_state) {
    case net_http2_req_state_write_closed:
    case net_http2_req_state_closed:
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: req %d: can`t write in req-state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            stream->m_stream_id, req->m_id, net_http2_req_state_str(req->m_state));
        return -1;
    default:
        break;
    }
    
    req->m_write_ctx = write_ctx;
    req->m_on_write = on_write;
    req->m_write_ctx_free = write_ctx_free;
    req->m_have_follow_data = have_follow_data;

    if (req->m_state == net_http2_req_state_established
        || req->m_state == net_http2_req_state_read_closed)
    {
        return net_http2_req_begin_write(req);
    }
    else {
        return 0;
    }
}

void net_http2_req_clear_writer(net_http2_req_t req) {
    req->m_write_ctx = NULL;
    req->m_on_write = NULL;
    req->m_write_ctx_free = NULL;
}

int net_http2_req_set_reader(
    net_http2_req_t req,
    void * read_ctx,
    net_http2_req_on_state_change_fun_t on_state_change,
    net_http2_req_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *))
{
    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (req->m_read_ctx_free) {
        req->m_read_ctx_free(req->m_read_ctx);
    }
    
    req->m_read_ctx = read_ctx;
    req->m_on_state_change = on_state_change;
    req->m_on_recv = on_recv;
    req->m_read_ctx_free = read_ctx_free;

    return 0;
}

void net_http2_req_clear_reader(net_http2_req_t req) {
    req->m_read_ctx = NULL;
    req->m_on_state_change = NULL;
    req->m_on_recv = NULL;
    req->m_read_ctx_free = NULL;
}

uint8_t net_http2_req_is_writing(net_http2_req_t req) {
    return req->m_on_write != NULL;
}

int net_http2_req_on_req_head_complete(net_http2_req_t req) {
    return 0;
}

void net_http2_req_set_stream(net_http2_req_t req, net_http2_stream_t stream) {
    if (req->m_stream == stream) return;

    if (req->m_stream) {
        assert(req->m_stream->m_req == req);
        req->m_stream->m_req = NULL;
        req->m_stream = NULL;
    }

    req->m_stream = stream;

    if (req->m_stream) {
        assert(req->m_stream->m_req == NULL);
        req->m_stream->m_req = req;
    }
}

void net_http2_req_set_req_state(net_http2_req_t req, net_http2_req_state_t state) {
    if (req->m_state == state) return;

    net_http2_endpoint_t endpoint = req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        if (req->m_stream) {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: http2: %d: req %d: req-state %s ==> %s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                req->m_stream->m_stream_id, req->m_id,
                net_http2_req_state_str(req->m_state), net_http2_req_state_str(state));
        } else {
            CPE_INFO(
                protocol->m_em, "http2: %s: %s: req %d: req-state %s ==> %s",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
                req->m_id,
                net_http2_req_state_str(req->m_state), net_http2_req_state_str(state));
        }
    }

    req->m_state = state;
}

const char *  net_http2_req_method_str(net_http2_req_method_t method) {
    switch(method) {
    case net_http2_req_method_get:
        return "GET";
    case net_http2_req_method_post:
        return "POST";
    case net_http2_req_method_head:
        return "HEAD";
    }
}

const char * net_http2_req_state_str(net_http2_req_state_t req_state) {
    switch(req_state) {
    case net_http2_req_state_init:
        return "init";
    case net_http2_req_state_connecting:
        return "connecting";
    case net_http2_req_state_head_sended:
        return "head-sended";
    case net_http2_req_state_head_received:
        return "head-receive";
    case net_http2_req_state_established:
        return "established";
    case net_http2_req_state_read_closed:
        return "read-closed";
    case net_http2_req_state_write_closed:
        return "write-closed";
    case net_http2_req_state_closed:
        return "closed";
    }
}
