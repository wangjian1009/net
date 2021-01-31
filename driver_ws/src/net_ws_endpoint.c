#include <assert.h>
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/random.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_ws_endpoint_i.h"
#include "net_ws_stream_endpoint_i.h"
#include "net_ws_utils.h"

extern struct wslay_event_callbacks s_net_ws_endpoint_callbacks;
static int net_ws_endpoint_set_runing_mode(net_ws_endpoint_t endpoint, net_ws_endpoint_runing_mode_t runing_mode);

net_ws_endpoint_t
net_ws_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_ws_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_ws_endpoint_stream(net_endpoint_t base_endpoint) {
    net_ws_endpoint_t endpoint = net_ws_endpoint_cast(base_endpoint);
    if (endpoint == NULL) return NULL;
    if (endpoint->m_stream == NULL) return NULL;
    return net_endpoint_from_data(endpoint->m_stream);
}

net_ws_endpoint_runing_mode_t net_ws_endpoint_runing_mode(net_ws_endpoint_t endpoint) {
    return endpoint->m_runing_mode;
}

net_ws_endpoint_state_t net_ws_endpoint_state(net_ws_endpoint_t endpoint) {
    return endpoint->m_state;
}

const char * net_ws_endpoint_path(net_ws_endpoint_t endpoint) {
    return endpoint->m_path;
}

int net_ws_endpoint_set_path(net_ws_endpoint_t endpoint, const char * path) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    if (endpoint->m_path) {
        mem_free(protocol->m_alloc, endpoint->m_path);
        endpoint->m_path = NULL;
    }

    if (path) {
        endpoint->m_path = cpe_str_mem_dup(protocol->m_alloc, path);
        if (endpoint->m_path == NULL) {
            net_endpoint_t base_endpoint =
                net_endpoint_from_protocol_data(
                    net_protocol_schedule(net_protocol_from_data(protocol)),
                    endpoint);
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: set path: dup failed",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
            return -1;
        }
    }
    
    return 0;
}

net_address_t net_ws_endpoint_host(net_ws_endpoint_t endpoint) {
    return endpoint->m_host;
}

int net_ws_endpoint_set_host(net_ws_endpoint_t endpoint, net_address_t host) {
    assert(endpoint->m_base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_address_cmp_opt(endpoint->m_host, host) == 0) return 0;

    net_address_t new_host = NULL;
    if (host) {
        new_host = net_address_copy(net_endpoint_schedule(endpoint->m_base_endpoint), host);
        if (new_host == NULL) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: set host: dup failed",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
            return -1;
        }
    }

    if (endpoint->m_host) {
        net_address_free(endpoint->m_host);
        endpoint->m_host = NULL;
    }

    endpoint->m_host = new_host;
    return 0;
}

uint8_t net_ws_endpoint_version(net_ws_endpoint_t endpoint) {
    return endpoint->m_version;
}

int net_ws_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_stream = NULL;

    endpoint->m_on_msg_text_ctx = NULL;
    endpoint->m_on_msg_text_fun = NULL;
    endpoint->m_on_msg_text_ctx_free = NULL;

    endpoint->m_on_msg_bin_ctx = NULL;
    endpoint->m_on_msg_bin_fun = NULL;
    endpoint->m_on_msg_bin_ctx_free = NULL;
    
    endpoint->m_path = NULL;
    endpoint->m_host = NULL;
    endpoint->m_state = net_ws_endpoint_state_init;
    endpoint->m_ws_ctx = NULL;
    
    return 0;
}

void net_ws_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_on_msg_text_ctx_free) {
        endpoint->m_on_msg_text_ctx_free(endpoint->m_on_msg_text_ctx);
        endpoint->m_on_msg_text_ctx_free = NULL;
    }
    endpoint->m_on_msg_text_ctx = NULL;
    endpoint->m_on_msg_text_fun = NULL;

    if (endpoint->m_on_msg_bin_ctx_free) {
        endpoint->m_on_msg_bin_ctx_free(endpoint->m_on_msg_bin_ctx);
        endpoint->m_on_msg_bin_ctx_free = NULL;
    }
    endpoint->m_on_msg_bin_ctx = NULL;
    endpoint->m_on_msg_bin_fun = NULL;
    
    if (endpoint->m_stream) {
        assert(endpoint->m_stream->m_underline == base_endpoint);
        endpoint->m_stream->m_underline = NULL;
        endpoint->m_stream = NULL;
    }

    if (endpoint->m_ws_ctx) {
        wslay_event_context_free(endpoint->m_ws_ctx);
        endpoint->m_ws_ctx = NULL;
    }

    if (endpoint->m_path) {
        mem_free(protocol->m_alloc, endpoint->m_path);
        endpoint->m_path = NULL;
    }
}

int net_ws_endpoint_input(net_endpoint_t base_endpoint) {
    net_ws_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_runing_mode == net_ws_endpoint_runing_mode_init
        || endpoint->m_state == net_ws_endpoint_state_init)
    {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: input in state %s.%s, error",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            net_ws_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_ws_endpoint_state_str(endpoint->m_state));
        return -1;
    }

    if (endpoint->m_state == net_ws_endpoint_state_handshake) {
        if (endpoint->m_runing_mode == net_ws_endpoint_runing_mode_svr) {
            if (net_ws_endpoint_input_handshake_svr(base_endpoint, endpoint) != 0) return -1;
        }
        else {
            assert(endpoint->m_runing_mode == net_ws_endpoint_runing_mode_cli);
            if (net_ws_endpoint_input_handshake_cli(base_endpoint, endpoint) != 0) return -1;
        }
        
        if (endpoint->m_state == net_ws_endpoint_state_handshake) return 0;
    }

    assert(endpoint->m_state == net_ws_endpoint_state_streaming);
    if (wslay_event_recv(endpoint->m_ws_ctx) != 0) {
        return -1;
    }

    return 0;
}

int net_ws_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_ws_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    net_endpoint_t base_stream =
        endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;
        
    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_resolving) != 0) return -1;
        }
        break;
    case net_endpoint_state_connecting:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1;
        }
        break;
    case net_endpoint_state_established:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) return -1;
        }
        if (net_ws_endpoint_send_handshake(base_endpoint, endpoint) != 0) return -1;
        break;
    case net_endpoint_state_error:
        if (base_stream) {
            net_endpoint_set_error(
                base_stream,
                net_endpoint_error_source(base_endpoint),
                net_endpoint_error_no(base_endpoint), net_endpoint_error_msg(base_endpoint));
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_error) != 0) return -1;
        }
        break;
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_disable:
    case net_endpoint_state_deleting:
        if (base_stream) {
            net_endpoint_set_error(
                base_stream,
                net_endpoint_error_source_network,
                net_endpoint_network_errno_logic,
                "endpoint ep state error");
        }
        return -1;
    }
    
    return 0;
}

int net_ws_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf)
{
    net_ws_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    switch(net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_endpoint_protocol_debug(base_endpoint)) {
            CPE_INFO(
                protocol->m_em, "net: ws: %s: write: cache %d data in state %s!",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                net_endpoint_buf_size(from_ep, from_buf),
                net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        }

        /* if (base_endpoint == from_ep) { */
        /*     return net_endpoint_buf_append_from_self( */
        /*         base_endpoint, net_ws_endpoint_ep_write_cache, from_buf, 0); */
        /* } */
        /* else { */
        /*     return net_endpoint_buf_append_from_other( */
        /*         base_endpoint, net_ws_endpoint_ep_write_cache, from_ep, from_buf, 0); */
        /* } */
        assert(0);
        return -1;
    case net_endpoint_state_established:
        switch(endpoint->m_state) {
        case net_ws_endpoint_state_init:
        case net_ws_endpoint_state_handshake:
            if (net_endpoint_protocol_debug(base_endpoint)) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: write: cache %d data in state %s.handshake!",
                    net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                    net_endpoint_buf_size(from_ep, from_buf),
                    net_endpoint_state_str(net_endpoint_state(base_endpoint)));
            }

            /* if (base_endpoint == from_ep) { */
            /*     return net_endpoint_buf_append_from_self( */
            /*         base_endpoint, net_ws_endpoint_ep_write_cache, from_buf, 0); */
            /* } */
            /* else { */
            /*     return net_endpoint_buf_append_from_other( */
            /*         base_endpoint, net_ws_endpoint_ep_write_cache, from_ep, from_buf, 0); */
            /* } */
            assert(0);
            return -1;
        case net_ws_endpoint_state_streaming:
            break;
        }
        break;
    case net_endpoint_state_error:
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
    case net_endpoint_state_deleting:
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: can`t write in state %s!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            net_endpoint_state_str(net_endpoint_state(base_endpoint)));
        return -1;
    }

    uint32_t data_size = net_endpoint_buf_size(from_ep, from_buf);
    if (data_size == 0) return 0;

    void * data = NULL;
    if (net_endpoint_buf_peak_with_size(from_ep, from_buf, data_size, &data) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: write: peak data fail!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    /* int r = SSL_write(endpoint->m_ws, data, data_size); */
    /* if (r < 0) { */
    /*     int err = SSL_get_error(endpoint->m_ws, r); */
    /*     return net_ws_endpoint_update_error(base_endpoint, err, r); */
    /* } */
    
    //net_endpoint_buf_consume(from_ep, from_buf, r);

    /* if (net_endpoint_protocol_debug(base_endpoint)) { */
    /*     CPE_INFO( */
    /*         protocol->m_em, "net: ws: %s: ==> %d data!", */
    /*         net_endpoint_dump(net_ws_driver_tmp_buffer(driver), base_endpoint), r); */
    /* } */
    
    return 0;
}

void net_ws_endpoint_set_msg_receiver_text(
    net_ws_endpoint_t endpoint,
    void * ctx, net_ws_endpoint_on_msg_text_fun_t fun, void (*ctx_free)(void*))
{
    if (endpoint->m_on_msg_text_ctx_free) {
        endpoint->m_on_msg_text_ctx_free(endpoint->m_on_msg_text_ctx);
        endpoint->m_on_msg_text_ctx_free = NULL;
    }

    endpoint->m_on_msg_text_ctx = ctx;
    endpoint->m_on_msg_text_fun = fun;
    endpoint->m_on_msg_text_ctx_free = ctx_free;
}

void net_ws_endpoint_set_msg_receiver_bin(
    net_ws_endpoint_t endpoint,
    void * ctx, net_ws_endpoint_on_msg_bin_fun_t fun, void (*ctx_free)(void*))
{
    if (endpoint->m_on_msg_bin_ctx_free) {
        endpoint->m_on_msg_bin_ctx_free(endpoint->m_on_msg_bin_ctx);
        endpoint->m_on_msg_bin_ctx_free = NULL;
    }

    endpoint->m_on_msg_bin_ctx = ctx;
    endpoint->m_on_msg_bin_fun = fun;
    endpoint->m_on_msg_bin_ctx_free = ctx_free;
}

static int net_ws_endpoint_set_runing_mode(net_ws_endpoint_t endpoint, net_ws_endpoint_runing_mode_t runing_mode) {
    if (endpoint->m_runing_mode == runing_mode) return 0;

    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "net: ws: %s: state %s ==> %s",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_endpoint_runing_mode_str(endpoint->m_runing_mode),
            net_ws_endpoint_runing_mode_str(endpoint->m_runing_mode));
    }

    if (endpoint->m_state != net_ws_endpoint_state_init) {
        if (net_ws_endpoint_set_state(endpoint, net_ws_endpoint_state_init) != 0) return -1;
    }

    endpoint->m_runing_mode = runing_mode;
    
    return 0;
}

int net_ws_endpoint_set_state(net_ws_endpoint_t endpoint, net_ws_endpoint_state_t state) {
    if (endpoint->m_state == state) return 0;

    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    assert(endpoint->m_runing_mode != net_ws_endpoint_runing_mode_init);
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint)) {
        CPE_INFO(
            protocol->m_em, "net: ws: %s: state %s ==> %s",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_endpoint_state_str(endpoint->m_state),
            net_ws_endpoint_state_str(state));
    }

    endpoint->m_state = state;

    switch(state) {
    case net_ws_endpoint_state_init:
        if (endpoint->m_ws_ctx) {
            wslay_event_context_free(endpoint->m_ws_ctx);
            endpoint->m_ws_ctx = NULL;
        }
        break;
    case net_ws_endpoint_state_handshake:
        assert(endpoint->m_ws_ctx == NULL);
        if (endpoint->m_runing_mode == net_ws_endpoint_runing_mode_cli) {
            endpoint->m_state_data.m_cli.m_handshake.m_state = net_ws_endpoint_handshake_cli_first_line;
            endpoint->m_state_data.m_cli.m_handshake.m_readed_size = 0;
            endpoint->m_state_data.m_cli.m_handshake.m_received_fields = 0;
        }
        else {
        }
        break;
    case net_ws_endpoint_state_streaming:
        assert(endpoint->m_ws_ctx == NULL);
        wslay_event_context_client_init(&endpoint->m_ws_ctx, &s_net_ws_endpoint_callbacks, endpoint);
        if (endpoint->m_ws_ctx == NULL) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: init context failed",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
            return -1;
        }
        break;
    }

    return 0;
}

const char * net_ws_endpoint_state_str(net_ws_endpoint_state_t state) {
    switch(state) {
    case net_ws_endpoint_state_init:
        return "init";
    case net_ws_endpoint_state_handshake:
        return "handshake";
    case net_ws_endpoint_state_streaming:
        return "streaming";
    }
}

const char * net_ws_endpoint_runing_mode_str(net_ws_endpoint_runing_mode_t runing_mode) {
    switch(runing_mode) {
    case net_ws_endpoint_runing_mode_init:
        return "init";
    case net_ws_endpoint_runing_mode_cli:
        return "cli";
    case net_ws_endpoint_runing_mode_svr:
        return "svr";
    }
}
