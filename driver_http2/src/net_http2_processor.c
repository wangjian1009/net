#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_http2_processor_i.h"
#include "net_http2_stream_i.h"

net_http2_processor_t
net_http2_processor_create(net_http2_endpoint_t endpoint) {
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    net_http2_processor_t processor = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_processor));
    if (processor == NULL) {
        CPE_ERROR(protocol->m_em, "http: processor: create: alloc fail!");
        return NULL;
    }

    processor->m_endpoint = endpoint;
    processor->m_stream = NULL;
    processor->m_head_count = 0;
    processor->m_head_capacity = 0;
    processor->m_headers = NULL;

    endpoint->m_processor_count++;
    TAILQ_INSERT_TAIL(&endpoint->m_processors, processor, m_next);
    
    return processor;
}

void net_http2_processor_free(net_http2_processor_t processor) {
    net_http2_endpoint_t endpoint = processor->m_endpoint;

    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    uint16_t i;
    for(i = 0; i < processor->m_head_count; ++i) {
        mem_free(protocol->m_alloc, processor->m_headers[i].m_name);
        mem_free(protocol->m_alloc, processor->m_headers[i].m_value);
    }
    processor->m_head_count = 0;
    
    if (processor->m_headers) {
        mem_free(protocol->m_alloc, processor->m_headers);
        processor->m_headers = NULL;
        processor->m_head_capacity = 0;
    }
            
    if (processor->m_stream) {
        net_http2_processor_set_stream(processor, NULL);
        assert(processor->m_stream == NULL);
    }
    
    assert(endpoint->m_processor_count > 0);
    endpoint->m_processor_count--;
    TAILQ_REMOVE(&endpoint->m_processors, processor, m_next);

    mem_free(protocol->m_alloc, processor);
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

int net_http2_processor_add_head(
    net_http2_processor_t processor,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len)
{
    net_http2_endpoint_t http_ep = processor->m_endpoint;
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (processor->m_head_count >= processor->m_head_capacity) {
        uint16_t new_capacity = processor->m_head_capacity < 8 ? 8 : processor->m_head_capacity * 2;
        struct net_http2_processor_pair * new_headers =
            mem_alloc(protocol->m_alloc, sizeof(struct net_http2_processor_pair) * new_capacity);
        if (new_headers == NULL) {
            CPE_ERROR(
                protocol->m_em, "http2: %s: req: add header: alloc nv faild, capacity=%d!",
                net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), new_capacity);
            return -1;
        }

        if (processor->m_headers) {
            memcpy(new_headers, processor->m_headers, sizeof(nghttp2_nv) * processor->m_head_count);
            mem_free(protocol->m_alloc, processor->m_headers);
        }

        processor->m_headers = new_headers;
        processor->m_head_capacity = new_capacity;
    }
    
    struct net_http2_processor_pair * nv = processor->m_headers + processor->m_head_count;
    nv->m_name = cpe_str_mem_dup_len(protocol->m_alloc, attr_name, name_len);
    if (nv->m_name == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: processor: add header: dup name %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_name);
        return -1;
    }

    nv->m_value = cpe_str_mem_dup_len(protocol->m_alloc, attr_value, value_len);
    if (nv->m_value == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: process: add header: dup value %s faild!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint), attr_value);
        mem_free(protocol->m_alloc, nv->m_name);
        return -1;
    }

    processor->m_head_count++;
    return 0;
}
