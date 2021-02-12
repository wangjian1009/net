#ifndef NET_HTTP2_PROTOCOL_I_H_INCLEDED
#define NET_HTTP2_PROTOCOL_I_H_INCLEDED
#include "nghttp2/nghttp2.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "net_http2_protocol.h"

typedef TAILQ_HEAD(net_http2_endpoint_list, net_http2_endpoint) net_http2_endpoint_list_t;
typedef TAILQ_HEAD(net_http2_stream_list, net_http2_stream) net_http2_stream_list_t;
typedef TAILQ_HEAD(net_http2_req_list, net_http2_req) net_http2_req_list_t;
typedef TAILQ_HEAD(net_http2_processor_list, net_http2_processor) net_http2_processor_list_t;

struct net_http2_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    nghttp2_session_callbacks * m_http2_callbacks;
    uint32_t m_max_req_id;
    struct mem_buffer m_data_buffer;
};

mem_buffer_t net_http2_protocol_tmp_buffer(net_http2_protocol_t protocol);

#endif
