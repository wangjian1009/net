#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/error.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http2_endpoint_i.h"
#include "net_http2_stream_endpoint_i.h"
#include "net_http2_stream_group_i.h"
#include "net_http2_stream_acceptor_i.h"
#include "net_http2_utils.h"

extern struct wslay_event_callbacks s_net_http2_endpoint_callbacks;

void net_http2_endpoint_free(net_http2_endpoint_t endpoint) {
    net_endpoint_free(endpoint->m_base_endpoint);
}

net_http2_endpoint_t
net_http2_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_http2_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_http2_endpoint_base_endpoint(net_http2_endpoint_t endpoint) {
    return endpoint->m_base_endpoint;
}

int net_http2_endpoint_init(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_runing_mode = net_http2_endpoint_runing_mode_init;

    endpoint->m_http2_session = NULL;
    if (nghttp2_session_client_new(&endpoint->m_http2_session, protocol->m_http2_callbacks, endpoint) != 0) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: nghttp2_session_client_new error",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    TAILQ_INIT(&endpoint->m_streams);
    
    return 0;
}

void net_http2_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    while(!TAILQ_EMPTY(&endpoint->m_streams)) {
        net_http2_stream_endpoint_t stream = TAILQ_FIRST(&endpoint->m_streams);
        net_http2_stream_endpoint_set_control(stream, NULL);
    }
    
    if (endpoint->m_http2_session) {
        nghttp2_session_del(endpoint->m_http2_session);
        endpoint->m_http2_session = NULL;
    }
}

int net_http2_endpoint_input(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    assert(net_endpoint_state(base_endpoint) != net_endpoint_state_deleting);
    
    /* uint32_t buf_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_read); */
    /* void * buf = NULL; */
    /* net_endpoint_buf_peak_with_size(base_endpoint, net_ep_buf_read, buf_size, &buf); */
    /* if (buf) { */
    /*     CPE_ERROR( */
    /*         protocol->m_em, "ws: input %s", */
    /*         mem_buffer_dump_data(net_http2_protocol_tmp_buffer(protocol), buf, buf_size, 0)); */
    /* } */

    return 0;
}

int net_http2_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    /* net_endpoint_t base_stream = */
    /*     endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL; */
        
    /* switch(net_endpoint_state(base_endpoint)) { */
    /* case net_endpoint_state_resolving: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_resolving) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_connecting: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_established: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1; */
    /*     } */
    /*     if (endpoint->m_runing_mode == net_http2_endpoint_runing_mode_cli) { */
    /*         if (net_http2_endpoint_send_handshake(base_endpoint, endpoint) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_error: */
    /*     if (base_stream) { */
    /*         net_endpoint_set_error( */
    /*             base_stream, */
    /*             net_endpoint_error_source(base_endpoint), */
    /*             net_endpoint_error_no(base_endpoint), net_endpoint_error_msg(base_endpoint)); */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_error) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_read_closed: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_read_closed) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_write_closed: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_write_closed) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_disable: */
    /*     if (base_stream) { */
    /*         if (net_endpoint_set_state(base_stream, net_endpoint_state_disable) != 0) return -1; */
    /*     } */
    /*     break; */
    /* case net_endpoint_state_deleting: */
    /*     break; */
    /* } */
    
    return 0;
}

net_http2_endpoint_runing_mode_t net_http2_endpoint_runing_mode(net_http2_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

void net_http2_endpoint_set_stream_group(
    net_http2_endpoint_t endpoint, net_http2_stream_group_t stream_group)
{
    assert(endpoint->m_runing_mode == net_http2_endpoint_runing_mode_cli);
    
    if (endpoint->m_cli.m_stream_group == stream_group) return;

    if (endpoint->m_cli.m_stream_group) {
        TAILQ_REMOVE(&endpoint->m_cli.m_stream_group->m_endpoints, endpoint, m_cli.m_next_for_group);
        endpoint->m_cli.m_stream_group = NULL;
    }

    endpoint->m_cli.m_stream_group = stream_group;

    if (endpoint->m_cli.m_stream_group) {
        TAILQ_INSERT_TAIL(&endpoint->m_cli.m_stream_group->m_endpoints, endpoint, m_cli.m_next_for_group);
    }
}

void net_http2_endpoint_set_stream_acceptor(
    net_http2_endpoint_t endpoint, net_http2_stream_acceptor_t stream_acceptor)
{
    assert(endpoint->m_runing_mode == net_http2_endpoint_runing_mode_svr);
    
    if (endpoint->m_svr.m_stream_acceptor == stream_acceptor) return;

    if (endpoint->m_svr.m_stream_acceptor) {
        TAILQ_REMOVE(&endpoint->m_svr.m_stream_acceptor->m_endpoints, endpoint, m_svr.m_next_for_acceptor);
        endpoint->m_svr.m_stream_acceptor = NULL;
    }

    endpoint->m_svr.m_stream_acceptor = stream_acceptor;

    if (endpoint->m_svr.m_stream_acceptor) {
        TAILQ_INSERT_TAIL(&endpoint->m_svr.m_stream_acceptor->m_endpoints, endpoint, m_svr.m_next_for_acceptor);
    }
}

int net_http2_endpoint_set_runing_mode(net_http2_endpoint_t endpoint, net_http2_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return 0;

    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "http2: %s: runing-mode: %s ==> %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_http2_endpoint_runing_mode_str(runing_mode));
    }

    switch(endpoint->m_runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        break;
    case net_http2_endpoint_runing_mode_cli:
        if (endpoint->m_cli.m_stream_group) {
            net_http2_endpoint_set_stream_group(endpoint, NULL);
            assert(endpoint->m_cli.m_stream_group == NULL);
        }
        break;
    case net_http2_endpoint_runing_mode_svr:
        break;
    }
    
    endpoint->m_runing_mode = runing_mode;

    switch(endpoint->m_runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        break;
    case net_http2_endpoint_runing_mode_cli:
        endpoint->m_cli.m_stream_group = NULL;
        break;
    case net_http2_endpoint_runing_mode_svr:
        endpoint->m_svr.m_stream_acceptor = NULL;
        break;
    }

    return 0;
}

const char * net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_http2_endpoint_runing_mode_init:
        return "init";
    case net_http2_endpoint_runing_mode_cli:
        return "cli";
    case net_http2_endpoint_runing_mode_svr:
        return "svr";
    }
}
