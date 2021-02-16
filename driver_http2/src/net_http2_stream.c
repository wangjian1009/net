#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_http2_stream_i.h"
#include "net_http2_req_i.h"

net_http2_stream_t
net_http2_stream_create(net_http2_endpoint_t endpoint, uint32_t stream_id, net_http2_stream_runing_mode_t runing_mode) {
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    net_http2_stream_t stream = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_stream));
    stream->m_endpoint = endpoint;
    stream->m_stream_id = stream_id;
    stream->m_runing_mode = runing_mode;
    stream->m_read_closed = 0;
    stream->m_write_closed = 0;
    stream->m_req = NULL;
    
    TAILQ_INSERT_TAIL(&endpoint->m_streams, stream, m_next_for_ep);
    
    return stream;
}

void net_http2_stream_free(net_http2_stream_t stream) {
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (stream->m_req) {
        net_http2_req_set_stream(stream->m_req, NULL);
        assert(stream->m_req == NULL);
    }

    TAILQ_REMOVE(&endpoint->m_streams, stream, m_next_for_ep);
    mem_free(protocol->m_alloc, stream);
}

uint32_t net_http2_stream_id(net_http2_stream_t stream) {
    return stream->m_stream_id;
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
    return stream->m_req;
}
