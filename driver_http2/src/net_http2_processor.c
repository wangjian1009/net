#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_http2_processor_i.h"
#include "net_http2_stream_i.h"

void net_http2_processor_set_state(net_http2_processor_t processor, net_http2_processor_state_t state);

net_http2_processor_t
net_http2_processor_create(net_http2_endpoint_t endpoint, uint32_t id) {
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    net_http2_processor_t processor = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_processor));
    if (processor == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: processor %d: create: alloc fail!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            processor->m_id);
        return NULL;
    }

    processor->m_endpoint = endpoint;
    processor->m_id = id;
    processor->m_stream = NULL;
    processor->m_state = net_http2_processor_state_init;
    processor->m_req_head_count = 0;
    processor->m_req_head_capacity = 0;
    processor->m_req_headers = NULL;
    processor->m_res_head_count = 0;
    processor->m_res_head_capacity = 0;
    processor->m_res_headers = NULL;

    endpoint->m_processor_count++;
    TAILQ_INSERT_TAIL(&endpoint->m_processors, processor, m_next);
    
    return processor;
}

void net_http2_processor_free(net_http2_processor_t processor) {
    net_http2_endpoint_t endpoint = processor->m_endpoint;

    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    /*req*/
    uint16_t i;
    for(i = 0; i < processor->m_req_head_count; ++i) {
        mem_free(protocol->m_alloc, processor->m_req_headers[i].m_name);
        mem_free(protocol->m_alloc, processor->m_req_headers[i].m_value);
    }
    processor->m_req_head_count = 0;
    
    if (processor->m_req_headers) {
        mem_free(protocol->m_alloc, processor->m_req_headers);
        processor->m_req_headers = NULL;
        processor->m_req_head_capacity = 0;
    }

    /*res*/
    for(i = 0; i < processor->m_res_head_count; ++i) {
        mem_free(protocol->m_alloc, processor->m_res_headers[i].name);
        mem_free(protocol->m_alloc, processor->m_res_headers[i].value);
    }
    processor->m_res_head_count = 0;
    
    if (processor->m_res_headers) {
        mem_free(protocol->m_alloc, processor->m_res_headers);
        processor->m_res_headers = NULL;
        processor->m_res_head_capacity = 0;
    }

    /*stream*/
    if (processor->m_stream) {
        net_http2_processor_set_stream(processor, NULL);
        assert(processor->m_stream == NULL);
    }
    
    assert(endpoint->m_processor_count > 0);
    endpoint->m_processor_count--;
    TAILQ_REMOVE(&endpoint->m_processors, processor, m_next);

    mem_free(protocol->m_alloc, processor);
}

net_http2_processor_state_t net_http2_processor_state(net_http2_processor_t processor) {
    return processor->m_state;
}

net_http2_endpoint_t net_http2_processor_endpoint(net_http2_processor_t processor) {
    return processor->m_endpoint;
}

net_http2_stream_t net_http2_processor_stream(net_http2_processor_t processor) {
    return processor->m_stream;
}

void net_http2_processor_set_stream(net_http2_processor_t processor, net_http2_stream_t stream) {
    if (processor->m_stream == stream) return;

    if (processor->m_stream) {
        assert(processor->m_stream->m_svr.m_processor == processor);
        processor->m_stream->m_svr.m_processor = NULL;
        processor->m_stream = NULL;
    }

    processor->m_stream = stream;

    if (processor->m_stream) {
        assert(processor->m_stream->m_runing_mode == net_http2_stream_runing_mode_svr);
        assert(processor->m_stream->m_svr.m_processor == NULL);
        processor->m_stream->m_svr.m_processor = processor;
    }
}

int net_http2_processor_add_req_head(
    net_http2_processor_t processor,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len)
{
    net_http2_endpoint_t http_ep = processor->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (processor->m_req_head_count >= processor->m_req_head_capacity) {
        uint16_t new_capacity = processor->m_req_head_capacity < 8 ? 8 : processor->m_req_head_capacity * 2;
        struct net_http2_processor_pair * new_headers =
            mem_alloc(protocol->m_alloc, sizeof(struct net_http2_processor_pair) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: %s: req: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint),
                net_http2_endpoint_runing_mode_str(http_ep->m_runing_mode),
                new_capacity);
            return -1;
        }

        if (processor->m_req_headers) {
            memcpy(new_headers, processor->m_req_headers, sizeof(nghttp2_nv) * processor->m_req_head_count);
            mem_free(protocol->m_alloc, processor->m_req_headers);
        }

        processor->m_req_headers = new_headers;
        processor->m_req_head_capacity = new_capacity;
    }
    
    struct net_http2_processor_pair * nv = processor->m_req_headers + processor->m_req_head_count;
    nv->m_name = cpe_str_mem_dup_len(protocol->m_alloc, attr_name, name_len);
    if (nv->m_name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: processor %d: add header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(http_ep->m_runing_mode),
            processor->m_id, attr_name);
        return -1;
    }

    nv->m_value = cpe_str_mem_dup_len(protocol->m_alloc, attr_value, value_len);
    if (nv->m_value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: process: %d: add header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(http_ep->m_runing_mode),
            processor->m_id, attr_value);
        mem_free(protocol->m_alloc, nv->m_name);
        return -1;
    }

    processor->m_req_head_count++;
    return 0;
}

const char * net_http2_processor_find_req_header(net_http2_processor_t processor, const char * name) {
    uint16_t i;
    for (i = 0; i < processor->m_req_head_count; ++i) {
        struct net_http2_processor_pair * header = processor->m_req_headers + i;
        if (strcmp(header->m_name, name) == 0) return header->m_value;
    }

    return NULL;
}

int net_http2_processor_add_res_head(net_http2_processor_t processor, const char * attr_name, const char * attr_value) {
    net_http2_endpoint_t http_ep = processor->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (processor->m_res_head_count >= processor->m_res_head_capacity) {
        uint16_t new_capacity = processor->m_res_head_capacity < 8 ? 8 : processor->m_res_head_capacity * 2;
        nghttp2_nv * new_headers = mem_alloc(protocol->m_alloc, sizeof(nghttp2_nv) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: processor: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), new_capacity);
            return -1;
        }

        if (processor->m_res_headers) {
            memcpy(new_headers, processor->m_res_headers, sizeof(nghttp2_nv) * processor->m_res_head_count);
            mem_free(protocol->m_alloc, processor->m_res_headers);
        }

        processor->m_res_headers = new_headers;
        processor->m_res_head_capacity = new_capacity;
    }
    
    nghttp2_nv * nv = processor->m_res_headers + processor->m_res_head_count;
    nv->namelen = strlen(attr_name);
    nv->name = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_name, nv->namelen);
    if (nv->name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: processor: add header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_name);
        return -1;
    }

    nv->valuelen = strlen(attr_value);
    nv->value = (void*)cpe_str_mem_dup_len(protocol->m_alloc, attr_value, nv->valuelen);
    if (nv->value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: processor: add header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_value);
        mem_free(protocol->m_alloc, nv->name);
        return -1;
    }

    nv->flags = NGHTTP2_NV_FLAG_NO_COPY_VALUE | NGHTTP2_NV_FLAG_NO_COPY_VALUE;

    processor->m_res_head_count++;
    return 0;
}

const char * net_http2_processor_find_res_header(net_http2_processor_t processor, const char * name) {
    uint16_t i;
    for (i = 0; i < processor->m_res_head_count; ++i) {
        nghttp2_nv * header = processor->m_res_headers + i;
        if (strcmp((const char *)header->name, name) == 0) return (const char *)header->value;
    }

    return NULL;
}

int net_http2_processor_start(
    net_http2_processor_t processor, void const * data, uint32_t data_size, uint8_t have_follow_data)
{
    net_http2_endpoint_t endpoint = processor->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (processor->m_state != net_http2_processor_state_head_received) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: processor %d: can`t start in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            processor->m_id, net_http2_processor_state_str(processor->m_state));
        return -1;
    }

    if (endpoint->m_state != net_http2_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: processor %d: start in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            processor->m_id,
            net_http2_endpoint_state_str(endpoint->m_state));
        return -1;
    }

    nghttp2_data_provider * data_prd = NULL;

    uint8_t flags = NGHTTP2_FLAG_NONE;
    if (!have_follow_data) {
        flags |= NGHTTP2_FLAG_END_STREAM;
    }

    int rv = nghttp2_submit_response(
        endpoint->m_http2_session, processor->m_stream->m_stream_id,
        processor->m_res_headers, processor->m_res_head_count, data_prd);
    if (rv < 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> submit processoruest fail, %s!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            processor->m_stream->m_stream_id,
            nghttp2_strerror(rv));
        return -1;
    }

    if (net_http2_endpoint_http2_flush(endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: http2: %d: ==> processor %d start failed!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            processor->m_stream->m_stream_id, processor->m_id);
        return -1;
    }
    
    net_http2_processor_set_state(processor, net_http2_processor_state_established);

    return 0;
}

int net_http2_processor_on_head_complete(net_http2_processor_t processor) {
    net_http2_endpoint_t endpoint = processor->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    net_http2_processor_set_state(processor, net_http2_processor_state_head_received);

    if (endpoint->m_accept_fun != NULL) {
        if (endpoint->m_accept_fun(endpoint->m_accept_ctx, processor) != 0) return -1;
    }

    return 0;
}

void net_http2_processor_set_state(net_http2_processor_t processor, net_http2_processor_state_t state) {
    if (processor->m_state == state) return;

    net_http2_endpoint_t endpoint = processor->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: %s: processor %d: state %s ==> %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            processor->m_id,
            net_http2_processor_state_str(processor->m_state), net_http2_processor_state_str(state));
    }

    processor->m_state = state;
}

const char * net_http2_processor_state_str(net_http2_processor_state_t state) {
    switch(state) {
    case net_http2_processor_state_init:
        return "init";
    case net_http2_processor_state_head_received:
        return "head-received";
    case net_http2_processor_state_established:
        return "established";
    case net_http2_processor_state_read_closed:
        return "read-closed";
    case net_http2_processor_state_write_closed:
        return "write-closed";
    case net_http2_processor_state_closed:
        return "closed";
    }
}
