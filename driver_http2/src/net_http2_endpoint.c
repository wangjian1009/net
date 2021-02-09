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
    endpoint->m_http2_session = NULL;

    return 0;
}

void net_http2_endpoint_fini(net_endpoint_t base_endpoint) {
    net_http2_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_http2_session) {
        //sfox_sfox_cli_router_http2_session_fini(endpoint);
        assert(endpoint->m_http2_session == NULL);
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
