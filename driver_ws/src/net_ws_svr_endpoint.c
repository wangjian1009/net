#include <assert.h>
#include "cpe/utils/error.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/sha1.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_endpoint.h"
#include "net_ws_svr_endpoint_i.h"
#include "net_ws_svr_stream_endpoint_i.h"

extern struct wslay_event_callbacks s_net_ws_svr_endpoint_callbacks;

static void net_ws_svr_endpoint_dump_error(net_endpoint_t base_endpoint, int val);
static int net_ws_svr_endpoint_update_error(net_endpoint_t base_endpoint, int err, int r);

net_ws_svr_endpoint_t net_ws_svr_endpoint_cast(net_endpoint_t base_endpoint) {
    net_protocol_t protocol = net_endpoint_protocol(base_endpoint);
    return net_protocol_endpoint_init_fun(protocol) == net_ws_svr_endpoint_init
        ? net_endpoint_protocol_data(base_endpoint)
        : NULL;
}

net_endpoint_t net_ws_svr_endpoint_stream(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_ws_svr_endpoint_cast(base_endpoint);
    if (endpoint == NULL) return NULL;
    if (endpoint->m_stream == NULL) return NULL;
    return net_endpoint_from_data(endpoint->m_stream);
}

net_ws_svr_endpoint_state_t net_ws_svr_endpoint_state(net_ws_svr_endpoint_t endpoint) {
    return endpoint->m_state;
}

const char * net_ws_svr_endpoint_path(net_ws_svr_endpoint_t endpoint) {
    return endpoint->m_path;
}

int net_ws_svr_endpoint_set_path(net_ws_svr_endpoint_t endpoint, const char * path) {
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    if (endpoint->m_path) {
        mem_free(protocol->m_alloc, endpoint->m_path);
        endpoint->m_path = NULL;
    }

    if (path) {
        endpoint->m_path = cpe_str_mem_dup(protocol->m_alloc, path);
        if (endpoint->m_path == NULL) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: set path: dup failed",
                net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
            return -1;
        }
    }
    
    return 0;
}

net_address_t net_ws_svr_endpoint_host(net_ws_svr_endpoint_t endpoint) {
    return endpoint->m_host;
}

int net_ws_svr_endpoint_set_host(net_ws_svr_endpoint_t endpoint, net_address_t host) {
    assert(endpoint->m_base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_address_cmp_opt(endpoint->m_host, host) == 0) return 0;

    net_address_t new_host = NULL;
    if (host) {
        new_host = net_address_copy(net_endpoint_schedule(endpoint->m_base_endpoint), host);
        if (new_host == NULL) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: set host: dup failed",
                net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
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

uint8_t net_ws_svr_endpoint_version(net_ws_svr_endpoint_t endpoint) {
    return endpoint->m_version;
}

int net_ws_svr_endpoint_init(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    endpoint->m_base_endpoint = base_endpoint;
    endpoint->m_stream = NULL;
    endpoint->m_host = NULL;
    endpoint->m_path = NULL;
    endpoint->m_version = 0;

    endpoint->m_on_msg_text_ctx = NULL;
    endpoint->m_on_msg_text_fun = NULL;
    endpoint->m_on_msg_text_ctx_free = NULL;

    endpoint->m_on_msg_bin_ctx = NULL;
    endpoint->m_on_msg_bin_fun = NULL;
    endpoint->m_on_msg_bin_ctx_free = NULL;
    
    endpoint->m_state = net_ws_svr_endpoint_state_handshake;
    endpoint->m_handshake.m_state = net_ws_svr_endpoint_handshake_first_line;
    endpoint->m_handshake.m_readed_size = 0;
    endpoint->m_handshake.m_received_fields = 0;

    wslay_event_context_server_init(&endpoint->m_ctx, &s_net_ws_svr_endpoint_callbacks, endpoint);
    if (endpoint->m_ctx == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: init: init context failed",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }
    
    return 0;
}

void net_ws_svr_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

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

    if (endpoint->m_ctx) {
        wslay_event_context_free(endpoint->m_ctx);
        endpoint->m_ctx = NULL;
    }
    
    if (endpoint->m_host) {
        net_address_free(endpoint->m_host);
        endpoint->m_host = NULL;
    }

    if (endpoint->m_path) {
        mem_free(protocol->m_alloc, endpoint->m_path);
        endpoint->m_path = NULL;
    }

}

int net_ws_svr_endpoint_input(net_endpoint_t base_endpoint) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_state == net_ws_svr_endpoint_state_handshake) {
        if (net_ws_svr_endpoint_input_handshake(base_endpoint, endpoint) != 0) return -1;
        if (endpoint->m_state == net_ws_svr_endpoint_state_handshake) return 0;
    }

    if (wslay_event_recv(endpoint->m_ctx) != 0) {
      return -1;
    }
    
    return 0;
}

int net_ws_svr_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state) {
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_endpoint_t base_stream = endpoint->m_stream ? net_endpoint_from_data(endpoint->m_stream) : NULL;
    
    switch (net_endpoint_state(base_endpoint)) {
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
    case net_endpoint_state_deleting:
        return 0;
    case net_endpoint_state_established:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_connecting) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_error:
        if (base_stream) {
            net_endpoint_set_error(
                base_stream, net_endpoint_error_source(base_stream),
                net_endpoint_error_no(base_stream), net_endpoint_error_msg(base_stream));

            if (net_endpoint_set_state(base_stream, net_endpoint_state_error) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            }
        }
        return 0;
    case net_endpoint_state_disable:
        if (base_stream) {
            if (net_endpoint_set_state(base_stream, net_endpoint_state_disable) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
                return 0;
            }
        }
        return 0;
    case net_endpoint_state_read_closed:
        return 0;
    case net_endpoint_state_write_closed:
        return 0;
    }
}

void net_ws_svr_endpoint_set_msg_receiver_text(
    net_ws_svr_endpoint_t endpoint,
    void * ctx, net_ws_svr_endpoint_on_msg_text_fun_t fun, void (*ctx_free)(void*))
{
    if (endpoint->m_on_msg_text_ctx_free) {
        endpoint->m_on_msg_text_ctx_free(endpoint->m_on_msg_text_ctx);
        endpoint->m_on_msg_text_ctx_free = NULL;
    }

    endpoint->m_on_msg_text_ctx = ctx;
    endpoint->m_on_msg_text_fun = fun;
    endpoint->m_on_msg_text_ctx_free = ctx_free;
}

void net_ws_svr_endpoint_set_msg_receiver_bin(
    net_ws_svr_endpoint_t endpoint,
    void * ctx, net_ws_svr_endpoint_on_msg_bin_fun_t fun, void (*ctx_free)(void*))
{
    if (endpoint->m_on_msg_bin_ctx_free) {
        endpoint->m_on_msg_bin_ctx_free(endpoint->m_on_msg_bin_ctx);
        endpoint->m_on_msg_bin_ctx_free = NULL;
    }

    endpoint->m_on_msg_bin_ctx = ctx;
    endpoint->m_on_msg_bin_fun = fun;
    endpoint->m_on_msg_bin_ctx_free = ctx_free;
}

const char * net_ws_svr_endpoint_state_str(net_ws_svr_endpoint_state_t state) {
    switch(state) {
    case net_ws_svr_endpoint_state_handshake:
        return "handshake";
    case net_ws_svr_endpoint_state_streaming:
        return "streaming";
    }
}
