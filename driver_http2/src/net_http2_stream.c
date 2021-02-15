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

int net_http2_stream_on_input(net_http2_stream_t stream, const uint8_t * data, uint32_t len) {
    /* if (net_endpoint_buf_append(stream->m_base_endpoint, net_ep_buf_read, data, (uint32_t)len) != 0) { */
    /*     if (net_endpoint_error_source(stream->m_base_endpoint) == net_endpoint_error_source_none) { */
    /*         net_endpoint_set_error( */
    /*             stream->m_base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL); */
    /*     } */
    /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_error) != 0) { */
    /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
    /*     } */
    /* } */
    return 0;
}

void net_http2_stream_on_close(net_http2_stream_t stream, int http2_error) {
}

void net_http2_stream_on_head_complete(net_http2_stream_t stream) {
                //TODO:
                /* if (endpoint->m_runing_mode == net_http2_endpoint_runing_mode_cli) { */
                /*     if (net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_established) != 0) { */
                /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
                /*     } */
                /* } */
                /* else { */
                /*     assert(endpoint->m_svr.m_stream_acceptor); */
                /*     if (net_http2_stream_acceptor_established(endpoint->m_svr.m_stream_acceptor, stream) != 0) { */
                /*         net_endpoint_set_state(stream->m_base_endpoint, net_endpoint_state_deleting); */
                /*     } */
                /* } */
}

int net_http2_stream_on_tailer(
    net_http2_stream_t stream,
    const char * name, uint32_t name_len, const char * value, uint32_t value_len)
{
    net_http2_endpoint_t endpoint = stream->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (cpe_str_cmp_part((const char *)name, name_len, "reason") == 0) {
        //assert(stream->m_conn);

        /* if (sfox_client_conn_close_reason(stream->m_conn) == sfox_client_conn_close_reason_none) { */
        /*     char buf[128]; */
            
        /*     sfox_client_conn_set_close_reason( */
        /*         stream->m_conn, */
        /*         sfox_client_conn_close_reason_remote_close, */
        /*         cpe_str_dup_len(buf, sizeof(buf), value, value_len), NULL); */
        /* } */
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
