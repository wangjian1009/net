#include <assert.h>
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/random.h"
#include "cpe/utils/stream_mem.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_ws_cli_endpoint_i.h"
#include "net_ws_cli_stream_endpoint_i.h"
#include "net_ws_utils.h"

extern struct wslay_event_callbacks s_net_ws_cli_endpoint_callbacks;

int net_ws_cli_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_cli_endpoint_t endpoint);

net_ws_cli_endpoint_t
net_ws_cli_endoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_ws_cli_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_ws_cli_endpoint_stream(net_endpoint_t base_endpoint) {
    net_ws_cli_endpoint_t endpoint = net_ws_cli_endoint_cast(base_endpoint);
    if (endpoint == NULL) return NULL;
    if (endpoint->m_stream == NULL) return NULL;
    return net_endpoint_from_data(endpoint->m_stream);
}

int net_ws_cli_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_cli_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_stream = NULL;
    endpoint->m_handshake_state = net_ws_cli_handshake_processing;
    endpoint->m_path = NULL;

    wslay_event_context_client_init(&endpoint->m_ctx, &s_net_ws_cli_endpoint_callbacks, endpoint);
    if (endpoint->m_ctx == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: init: init context failed",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }
    
    return 0;
}

void net_ws_cli_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_cli_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    
    if (endpoint->m_stream) {
        assert(endpoint->m_stream->m_underline == base_endpoint);
        endpoint->m_stream->m_underline = NULL;
        endpoint->m_stream = NULL;
    }

    if (endpoint->m_ctx) {
        wslay_event_context_free(endpoint->m_ctx);
        endpoint->m_ctx = NULL;
    }

    if (endpoint->m_path) {
        mem_free(protocol->m_alloc, endpoint->m_path);
        endpoint->m_path = NULL;
    }
}

int net_ws_cli_endpoint_input(net_endpoint_t base_endpoint) {
    net_ws_cli_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    return 0;
}

int net_ws_cli_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_ws_cli_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_cli_endpoint_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    net_endpoint_t base_stream =
        endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;
        
    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_resolving) != 0) return -1;
        }
        break;
    case net_endpoint_state_connecting:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        }
        break;
    case net_endpoint_state_established:
        if (base_endpoint) {
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting) != 0) return -1;
        }
        if (net_ws_cli_endpoint_send_handshake(base_endpoint, endpoint) != 0) return -1;
        break;
    case net_endpoint_state_error:
        if (base_endpoint) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source(base_endpoint),
                net_endpoint_error_no(base_endpoint), net_endpoint_error_msg(base_endpoint));
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) return -1;
        }
        break;
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        if (base_endpoint) {
            net_endpoint_set_error(
                base_endpoint,
                net_endpoint_error_source_network,
                net_endpoint_network_errno_logic,
                "endpoint ep state error");
        }
        return -1;
    }
    
    return 0;
}

int net_ws_cli_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    net_ws_cli_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "net: ws: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }

        /* if (base_endpoint == from_ep) { */
        /*     return net_endpoint_buf_append_from_self( */
        /*         base_endpoint, net_ws_cli_endpoint_ep_write_cache, from_buf, 0); */
        /* } */
        /* else { */
        /*     return net_endpoint_buf_append_from_other( */
        /*         base_endpoint, net_ws_cli_endpoint_ep_write_cache, from_ep, from_buf, 0); */
        /* } */
        assert(0);
        return -1;
    case net_endpoint_state_established:
        switch(endpoint->m_handshake_state) {
        case net_ws_cli_handshake_processing:
            if (net_endpoint_protocol_debug(base_endpoint)) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_endpoint)));
            }

            /* if (base_endpoint == from_ep) { */
            /*     return net_endpoint_buf_append_from_self( */
            /*         base_endpoint, net_ws_cli_endpoint_ep_write_cache, from_buf, 0); */
            /* } */
            /* else { */
            /*     return net_endpoint_buf_append_from_other( */
            /*         base_endpoint, net_ws_cli_endpoint_ep_write_cache, from_ep, from_buf, 0); */
            /* } */
            assert(0);
            return -1;
        case net_ws_cli_handshake_done:
            break;
        }
        break;
    case net_endpoint_state_error:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: peak data fail!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    /* int r = SSL_write(endpoint->m_ws, data, data_size); */
    /* if (r < 0) { */
    /*     int err = SSL_get_error(endpoint->m_ws, r); */
    /*     return net_ws_cli_endpoint_update_error(base_endpoint, err, r); */
    /* } */
    
    //net_endpoint_buf_consume(from_ep, from_buf, r);

    /* if (net_endpoint_protocol_debug(base_endpoint)) { */
    /*     CPE_INFO( */
    /*         protocol->m_em, "net: ws: %s: ==> %d data!", */
    /*         net_endpoint_dump(net_ws_cli_driver_tmp_buffer(driver), base_endpoint), r); */
    /* } */
    
    return 0;
}

int net_ws_cli_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_cli_endpoint_t endpoint) {
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_path == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no path!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no remote address!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    uint32_t buf_capacity = 4096;
    char * buf = net_endpoint_buf_alloc_at_least(base_endpoint, &buf_capacity);
    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, buf_capacity);
    
    stream_printf((write_stream_t)&ws, "GET %s HTTP/1.1\r\n", endpoint->m_path);

    stream_printf((write_stream_t)&ws, "Host: ");
    net_address_print((write_stream_t)&ws, target_addr);
    stream_printf((write_stream_t)&ws, "\r\n");
    
    stream_printf((write_stream_t)&ws, "Upgrade: websocket\r\n");
    stream_printf((write_stream_t)&ws, "Connection: Upgrade\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Key: ");
    char client_key[16];
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), client_key, sizeof(client_key));
    cpe_base64_encode_from_buf((write_stream_t)&ws, client_key, sizeof(client_key));
    stream_printf((write_stream_t)&ws, "\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Version: 13\r\n");
    stream_printf((write_stream_t)&ws, "\r\n");

    uint32_t size = ws.m_pos;

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data\n%.*s",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
            size, size, buf);
        return -1;
    }
    
    if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_write, size) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data failed!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
            size);
        return -1;
    }

    return 0;
}
