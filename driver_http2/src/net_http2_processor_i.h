#ifndef NET_HTTP2_PROCESSOR_I_H_INCLEDED
#define NET_HTTP2_PROCESSOR_I_H_INCLEDED
#include "net_http2_processor.h"
#include "net_http2_endpoint_i.h"

struct net_http2_processor_pair {
    char * m_name;
    char * m_value;
};

struct net_http2_processor {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_processor) m_next;
    net_http2_processor_state_t m_state;
    net_http2_stream_t m_stream;
    uint16_t m_head_count;
    uint16_t m_head_capacity;
    struct net_http2_processor_pair * m_headers;
};

net_http2_processor_t net_http2_processor_create(net_http2_endpoint_t endpoint);
void net_http2_processor_free(net_http2_processor_t processor);

int net_http2_processor_add_head(
    net_http2_processor_t processor,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len);

void net_http2_processor_set_stream(net_http2_processor_t processor, net_http2_stream_t stream);

int net_http2_processor_on_head_complete(net_http2_processor_t processor);

#endif
