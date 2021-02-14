#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_http2_processor_i.h"
#include "net_http2_stream_i.h"

net_http2_processor_t
net_http2_processor_create(net_http2_stream_t stream) {
    /* net_http2_protocol_t protocol = */
    /*     net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint)); */

    
    /* net_http2_processor_t processor = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_processor)); */
    /* if (processor == NULL) { */
    /*     CPE_ERROR(protocol->m_em, "http: processor: create: alloc fail!"); */
    /*     return NULL; */
    /* } */

    /* processor->m_endpoint = http_ep; */
    /* http_ep->m_processor_count++; */
    /* TAILQ_INSERT_TAIL(&http_ep->m_processors, processor, m_next); */
    
    /* return processor; */
    return NULL;
}

void net_http2_processor_free(net_http2_processor_t processor) {
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
