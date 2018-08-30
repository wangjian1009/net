#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/random.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/sha1.h"
#include "net_endpoint.h"
#include "net_http_endpoint.h"
#include "net_http_req.h"
#include "net_http_protocol.h"
#include "net_address.h"
#include "net_ws_endpoint_i.h"
#include "net_ws_protocol_i.h"

static int net_ws_endpoint_do_process(net_ws_endpoint_t ws_ep);
static void net_ws_endpoint_reset_data(net_ws_endpoint_t ws_ep);
static int net_ws_endpoint_send_handshake(net_ws_endpoint_t ws_ep);

static net_http_res_op_result_t net_ws_endpoint_on_handshake_rcode(void * ctx, net_http_req_t req, uint16_t code, const char * msg);
static net_http_res_op_result_t net_ws_endpoint_on_handshake_head(void * ctx, net_http_req_t req, const char * name, const char * value);
static net_http_res_op_result_t net_ws_endpoint_on_handshake_complete(void * ctx, net_http_req_t req, net_http_res_result_t result);

static int net_ws_endpoint_notify_state_changed(net_ws_endpoint_t ws_ep, net_ws_state_t old_state);

net_ws_endpoint_t
net_ws_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_ws_protocol_t ws_protocol) {
    net_http_endpoint_t http_ep = net_http_endpoint_create(driver, type, net_http_protocol_from_data(ws_protocol));
    if (http_ep == NULL) return NULL;
    
    net_ws_endpoint_t ws_ep = net_http_endpoint_data(http_ep);

    ws_ep->m_http_ep = http_ep;
    
    return ws_ep;
}

void net_ws_endpoint_free(net_ws_endpoint_t ws_ep) {
    return net_http_endpoint_free(ws_ep->m_http_ep);
}

net_ws_endpoint_t net_ws_endpoint_get(net_endpoint_t endpoint) {
    return net_http_endpoint_data(net_endpoint_protocol_data(endpoint));
}

net_ws_protocol_t net_ws_endpoint_protocol(net_ws_endpoint_t ws_ep) {
    return net_http_protocol_data(net_http_endpoint_protocol(net_http_endpoint_from_data(ws_ep)));
}

uint32_t net_ws_endpoint_reconnect_span_ms(net_ws_endpoint_t ws_ep) {
    return net_http_endpoint_reconnect_span_ms(ws_ep->m_http_ep);
}

void net_http_ws_set_reconnect_span_ms(net_ws_endpoint_t ws_ep, uint32_t span_ms) {
    net_http_endpoint_set_reconnect_span_ms(ws_ep->m_http_ep, span_ms);
}

uint8_t net_ws_endpoint_use_https(net_ws_endpoint_t ws_ep) {
    return net_http_endpoint_use_https(ws_ep->m_http_ep);
}

int net_ws_endpoint_set_use_https(net_ws_endpoint_t ws_ep, uint8_t use_https) {
    if (use_https) {
        if (net_http_endpoint_ssl_enable(ws_ep->m_http_ep) != 0) return -1;
    }
    else {
        if (net_http_endpoint_ssl_disable(ws_ep->m_http_ep) != 0) return -1;
    }

    return 0;
}

void * net_ws_endpoint_data(net_ws_endpoint_t ws_ep) {
    return ws_ep + 1;
}

net_ws_endpoint_t net_ws_endpoint_from_data(void * data) {
    return ((net_ws_endpoint_t)data) - 1;
}

net_ws_state_t net_ws_endpoint_state(net_ws_endpoint_t ws_ep) {
    return ws_ep->m_state;
}

net_http_endpoint_t net_ws_endpoint_http_ep(net_ws_endpoint_t ws_ep) {
    return ws_ep->m_http_ep;
}

net_endpoint_t net_ws_endpoint_net_ep(net_ws_endpoint_t ws_ep) {
    return net_http_endpoint_net_ep(ws_ep->m_http_ep);
}

int net_ws_endpoint_set_remote_and_path(net_ws_endpoint_t ws_ep, const char * url) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));

    const char * addr_begin;
    if (cpe_str_start_with(url, "ws://")) {
        if (net_http_endpoint_ssl_disable(ws_ep->m_http_ep) != 0) return -1;
        addr_begin = url + 5;
    }
    else if (cpe_str_start_with(url, "wss://")) {
        if (net_http_endpoint_ssl_enable(ws_ep->m_http_ep) != 0) return -1;
        addr_begin = url + 6;
    }
    else {
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: set remote and path: url %s check protocol fail!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            url);
        return -1;
    }

    const char * addr_end = strchr(addr_begin, '/');
    const char * str_address = NULL;
    const char * path = NULL;
    
    if (addr_end) {
        mem_buffer_t tmp_buffer = net_ws_protocol_tmp_buffer(ws_protocol);
        mem_buffer_clear_data(tmp_buffer);
        str_address = mem_buffer_strdup_range(tmp_buffer, addr_begin, addr_end);
        if (str_address == NULL) {
            CPE_ERROR(
                ws_protocol->m_em,
                "ws: %s: set remote and path: url %s address dup fail!",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                url);
            return -1;
        }
        path = addr_end;
    }
    else {
        str_address = addr_begin;
    }

    net_address_t address = net_address_create_auto(net_endpoint_schedule(net_http_endpoint_net_ep(ws_ep->m_http_ep)), str_address);
    if (address == NULL) {
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: set remote and path: url %s address format error!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            url);
        return -1;
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, net_http_endpoint_use_https(ws_ep->m_http_ep) ? 443 : 80);
    }
        
    if (net_endpoint_set_remote_address(
            net_http_endpoint_net_ep(ws_ep->m_http_ep), address, 1) != 0) {
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: set remote and path: url %s set remote address fail!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            url);
        net_address_free(address);
        return -1;
    }

    return net_ws_endpoint_set_path(ws_ep, path);
}

const char * net_ws_endpoint_path(net_ws_endpoint_t ws_ep) {
    return ws_ep->m_cfg_path ? ws_ep->m_cfg_path : "/";
}

int net_ws_endpoint_set_path(net_ws_endpoint_t ws_ep, const char * path) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));

    if (ws_ep->m_cfg_path) {
        mem_free(ws_protocol->m_alloc, ws_ep->m_cfg_path);
        ws_ep->m_cfg_path = NULL;
    }

    if (path) {
        ws_ep->m_cfg_path = cpe_str_mem_dup(ws_protocol->m_alloc, path);
        if (ws_ep->m_cfg_path == NULL) {
            CPE_ERROR(
                ws_protocol->m_em,
                "ws: %s: set path: copy path %s fail!",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                path);
            return -1;
        }
    }
    
    return 0;
}

int net_ws_endpoint_send_msg_text(net_ws_endpoint_t ws_ep, const char * msg) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
    struct wslay_event_msg ws_msg = { WSLAY_TEXT_FRAME,  (const uint8_t *)msg, strlen(msg) };

    if (wslay_event_queue_msg(ws_ep->m_ctx, &ws_msg) != 0) {
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: send msg text: queue msg fail!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        return -1;
    }

    if (net_endpoint_protocol_debug(net_http_endpoint_net_ep(ws_ep->m_http_ep)) >= 2) {
        CPE_INFO(
            ws_protocol->m_em, "ws: %s: msg text: >>>\n%s",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            msg);
    }

    if (net_ws_endpoint_do_process(ws_ep) != 0) return -1;
    
    return 0;
}

int net_ws_endpoint_send_msg_bin(net_ws_endpoint_t ws_ep, const void * msg, uint32_t msg_len) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
    struct wslay_event_msg ws_msg = { WSLAY_BINARY_FRAME,  (const uint8_t *)msg, msg_len };

    if (wslay_event_queue_msg(ws_ep->m_ctx, &ws_msg) != 0) {
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: send msg text: queue bintray fail, len=%d!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)), msg_len);
        return -1;
    }

    if (net_endpoint_protocol_debug(net_http_endpoint_net_ep(ws_ep->m_http_ep)) >= 2) {
        CPE_INFO(
            ws_protocol->m_em, "ws: %s: msg bin: >>> %d data",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            msg_len);
    }

    if (net_ws_endpoint_do_process(ws_ep) != 0) return -1;
        
    return 0;
}

void net_ws_endpoint_enable(net_ws_endpoint_t ws_ep) {
    net_http_endpoint_enable(ws_ep->m_http_ep);
}

int net_ws_endpoint_set_state(net_ws_endpoint_t ws_ep, net_ws_state_t state) {
    if (ws_ep->m_state == state) return 0;
    
    if (net_endpoint_protocol_debug(net_http_endpoint_net_ep(ws_ep->m_http_ep)) >= 1) {
        net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
        CPE_INFO(
            ws_protocol->m_em, "ws: %s: state %s ==> %s",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            net_ws_state_str(ws_ep->m_state),
            net_ws_state_str(state));
    }

    net_ws_state_t old_state = ws_ep->m_state;
    
    ws_ep->m_state = state;

    return net_ws_endpoint_notify_state_changed(ws_ep, old_state);
}

static int net_ws_endpoint_notify_state_changed(net_ws_endpoint_t ws_ep, net_ws_state_t old_state) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
        
    int rv = ws_protocol->m_endpoint_on_state_change
        ? ws_protocol->m_endpoint_on_state_change(ws_ep, old_state)
        : 0;
    
    /* net_http_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors); */
    /* while(monitor) { */
    /*     net_http_endpoint_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next); */
    /*     assert(!monitor->m_is_free); */
    /*     assert(!monitor->m_is_processing); */

    /*     if (monitor->m_on_state_change) { */
    /*         monitor->m_is_processing = 1; */
    /*         monitor->m_on_state_change(monitor->m_ctx, endpoint, old_state); */
    /*         monitor->m_is_processing = 0; */
    /*         if (monitor->m_is_free) net_http_endpoint_monitor_free(monitor); */
    /*     } */
        
    /*     monitor = next_monitor; */
    /* } */

    return rv;
}

static ssize_t net_ws_endpoint_recv_cb(
    wslay_event_context_ptr ctx, uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_endpoint_t ws_ep = (net_ws_endpoint_t)user_data;

    uint32_t size = (uint32_t)len;
    if (net_endpoint_buf_recv(net_http_endpoint_net_ep(ws_ep->m_http_ep), net_ep_buf_read, data, &size) != 0) {
        net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: rbuf recv fail!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    return (ssize_t)size;
}

static ssize_t net_ws_endpoint_send_cb(
    wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_endpoint_t ws_ep = (net_ws_endpoint_t)user_data;

    if (net_endpoint_buf_append(net_http_endpoint_net_ep(ws_ep->m_http_ep), net_ep_buf_write, data, (uint32_t)len) != 0) {
        net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
        CPE_ERROR(
            ws_protocol->m_em,
            "ws: %s: wbuf append fail!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    
    return (size_t)len;
}

static int net_ws_endpoint_genmask_cb(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), buf, len);
    return 0;
}

static void net_ws_endpoint_on_msg_recv_cb(
    wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    net_ws_endpoint_t ws_ep = user_data;
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
    
    switch(arg->opcode) {
    case WSLAY_CONTINUATION_FRAME:
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: continuation: not support!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        break;
    case WSLAY_TEXT_FRAME: {
        mem_buffer_clear_data(&ws_protocol->m_data_buffer);
        char * buf = mem_buffer_alloc(&ws_protocol->m_data_buffer, arg->msg_length + 1);
        if (buf == NULL) {
            CPE_ERROR(
                ws_protocol->m_em, "ws: %s: msg text: alloc buf fail, msg-length=%d!",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                (int)arg->msg_length);
            break;
        }
        memcpy(buf, arg->msg, arg->msg_length);
        buf[arg->msg_length] = 0;

        if (net_endpoint_protocol_debug(net_http_endpoint_net_ep(ws_ep->m_http_ep)) >= 2) {
            CPE_INFO(
                ws_protocol->m_em, "ws: %s: msg text: <<<\n%s",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                buf);
        }
        
        if (ws_protocol->m_endpoint_on_text_msg) {
            if (ws_protocol->m_endpoint_on_text_msg(ws_ep, buf) != 0) {
                wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
            }
        }
        break;
    }
    case WSLAY_BINARY_FRAME:
        if (net_endpoint_protocol_debug(net_http_endpoint_net_ep(ws_ep->m_http_ep)) >= 2) {
            CPE_INFO(
                ws_protocol->m_em, "ws: %s: msg bin: <<< %d data",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                (int)arg->msg_length);
        }
        
        if (ws_protocol->m_endpoint_on_bin_msg) {
            if (ws_protocol->m_endpoint_on_bin_msg(ws_ep, arg->msg, (uint32_t)arg->msg_length) != 0) {
                wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
            }
        }
        break;
    case WSLAY_CONNECTION_CLOSE:
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: close: not support!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        break;
    case WSLAY_PING:
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: ping: not support!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        break;
    case WSLAY_PONG:
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: pong: not support!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        break;
    }
}

static struct wslay_event_callbacks s_net_ws_endpoint_callbacks = {
    net_ws_endpoint_recv_cb,
    net_ws_endpoint_send_cb,
    net_ws_endpoint_genmask_cb,
    NULL, /* on_frame_recv_start_callback */
    NULL, /* on_frame_recv_callback */
    NULL, /* on_frame_recv_end_callback */
    net_ws_endpoint_on_msg_recv_cb
};

int net_ws_endpoint_init(net_http_endpoint_t http_ep) {
    net_ws_endpoint_t ws_ep = net_http_endpoint_data(http_ep);
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(http_ep));

    ws_ep->m_http_ep = NULL;
    ws_ep->m_state = net_ws_state_init;
    ws_ep->m_cfg_path = NULL;
    ws_ep->m_handshake_token = NULL;

    wslay_event_context_client_init(&ws_ep->m_ctx, &s_net_ws_endpoint_callbacks, ws_ep);

    if (ws_protocol->m_endpoint_init && ws_protocol->m_endpoint_init(ws_ep) != 0) {
        CPE_ERROR(ws_protocol->m_em, "ws: ???: init: external init fail!");
        return -1;
    }
    
    return 0;
}

void net_ws_endpoint_fini(net_http_endpoint_t http_ep) {
    net_ws_endpoint_t ws_ep = net_http_endpoint_data(http_ep);
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(http_ep));

    if (ws_protocol->m_endpoint_fini) {
        ws_protocol->m_endpoint_fini(ws_ep);
    }
    
    if (ws_ep->m_ctx) {
        wslay_event_context_free(ws_ep->m_ctx);
        ws_ep->m_ctx = NULL;
    }

    if (ws_ep->m_handshake_token) {
        mem_free(ws_protocol->m_alloc, ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    if (ws_ep->m_cfg_path) {
        mem_free(ws_protocol->m_alloc, ws_ep->m_cfg_path);
        ws_ep->m_cfg_path = NULL;
    }
    
    ws_ep->m_http_ep = NULL;
}

int net_ws_endpoint_input(net_http_endpoint_t http_ep) {
    net_ws_endpoint_t ws_ep = net_http_endpoint_data(http_ep);

    if (net_ws_endpoint_do_process(ws_ep) != 0) return -1;
        
    return ws_ep->m_state == net_ws_state_established ? 0 : -1;
}

int net_ws_endpoint_on_state_change(net_http_endpoint_t http_ep, net_http_state_t old_state) {
    net_ws_endpoint_t ws_ep = net_http_endpoint_data(http_ep);

    switch(net_http_endpoint_state(http_ep)) {
    case net_http_state_disable:
        if (net_ws_endpoint_set_state(ws_ep, net_ws_state_init) != 0) return -1;
        net_ws_endpoint_reset_data(ws_ep);
        break;
    case net_http_state_error:
        if (net_ws_endpoint_set_state(ws_ep, net_ws_state_error) != 0) return -1;
        net_ws_endpoint_reset_data(ws_ep);
        break;
    case net_http_state_connecting:
        if (net_ws_endpoint_set_state(ws_ep, net_ws_state_connecting) != 0) return -1;
        break;
    case net_http_state_established:
        if (net_ws_endpoint_send_handshake(ws_ep) != 0) {
            if (net_ws_endpoint_set_state(ws_ep, net_ws_state_error) != 0) return -1;
        }
        else {
            if (net_ws_endpoint_set_state(ws_ep, net_ws_state_handshake) != 0) return -1;
        }
        
        break;
    }

    return 0;
}

static int net_ws_endpoint_do_process(net_ws_endpoint_t ws_ep) {

    if (wslay_event_want_read(ws_ep->m_ctx)
        && !net_endpoint_buf_is_empty(net_http_endpoint_net_ep(ws_ep->m_http_ep), net_ep_buf_read))
    {
        if (wslay_event_recv(ws_ep->m_ctx) != 0) {
            net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
            CPE_ERROR(
                ws_protocol->m_em, "ws: %s: process: recv fail, auto disconnect",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(ws_protocol),
                    net_http_endpoint_net_ep(ws_ep->m_http_ep)));
            return -1;
        }
    }

    if (wslay_event_want_write(ws_ep->m_ctx)
        && !net_endpoint_buf_is_full(net_http_endpoint_net_ep(ws_ep->m_http_ep), net_ep_buf_write))
    {
        if (wslay_event_send(ws_ep->m_ctx) != 0) {
            net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));
            CPE_ERROR(
                ws_protocol->m_em, "ws: %s: process: send fail, auto disconnect",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)));
            return -1;
        }
    }

    return 0;
}

static void net_ws_endpoint_reset_data(net_ws_endpoint_t ws_ep) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));

    if (ws_ep->m_handshake_token) {
        mem_free(ws_protocol->m_alloc, ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    wslay_event_context_free(ws_ep->m_ctx);
    wslay_event_context_client_init(&ws_ep->m_ctx, &s_net_ws_endpoint_callbacks, ws_ep);
}

static int net_ws_endpoint_send_handshake(net_ws_endpoint_t ws_ep) {
    net_ws_protocol_t ws_protocol = net_http_protocol_data(net_http_endpoint_protocol(ws_ep->m_http_ep));

    /*生成token */
    if (ws_ep->m_handshake_token) {
        mem_free(ws_protocol->m_alloc, ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    char rand_buf[16];
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), rand_buf, sizeof(rand_buf));
    ws_ep->m_handshake_token =
        cpe_str_mem_dup(
            ws_protocol->m_alloc, 
            cpe_base64_dump(net_ws_protocol_tmp_buffer(ws_protocol), rand_buf, sizeof(rand_buf)));
    if (ws_ep->m_handshake_token == NULL) {
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s:    handshake: generate token fail",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        return -1;
    }

    net_http_req_t http_req = net_http_req_create(ws_ep->m_http_ep, net_http_req_method_get, net_ws_endpoint_path(ws_ep));
    if (http_req == NULL) {
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s:    handshake: create http-req fail",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        return -1;
    }

    if (net_http_req_write_head_host(http_req) != 0
        || net_http_req_write_head_pair(http_req, "Upgrade", "websocket") != 0
        || net_http_req_write_head_pair(http_req, "Connection", "Upgrade") != 0
        || net_http_req_write_head_pair(http_req, "Sec-WebSocket-Key", ws_ep->m_handshake_token) != 0
        || net_http_req_write_head_pair(http_req, "Sec-WebSocket-Version", "13") != 0
        || net_http_req_write_commit(http_req) != 0
        || net_http_req_set_reader(
            http_req,
            ws_ep,
            net_ws_endpoint_on_handshake_rcode,
            net_ws_endpoint_on_handshake_head,
            NULL,
            net_ws_endpoint_on_handshake_complete) != 0
        )
    {
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: handshake: write fail",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        return -1;
    }

    return 0;
}

static net_http_res_op_result_t net_ws_endpoint_on_handshake_rcode(void * ctx, net_http_req_t req, uint16_t code, const char * msg) {
    net_ws_endpoint_t ws_ep = ctx;
    net_ws_protocol_t ws_protocol = net_ws_endpoint_protocol(ws_ep);
    
    if (code != 101) {
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: handshake response: connect error, error=%d (%s)",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)),
            code, msg);
        return net_http_res_op_error_and_reconnect;
    }

    return net_http_res_op_success;
}

static net_http_res_op_result_t net_ws_endpoint_on_handshake_head(void * ctx, net_http_req_t req, const char * name, const char * value) {
    net_ws_endpoint_t ws_ep = ctx;
    net_ws_protocol_t ws_protocol = net_ws_endpoint_protocol(ws_ep);

    if (strcasecmp(name, "sec-websocket-accept") == 0) {
        char accept_token_buf[64];
        snprintf(accept_token_buf, sizeof(accept_token_buf), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_ep->m_handshake_token);

        struct cpe_sha1_value sha1_value;
        if (cpe_sha1_encode_str(&sha1_value, accept_token_buf) != 0) {
            CPE_ERROR(
                ws_protocol->m_em, "ws: %s: handshake response: calc accept token fail",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)));
            return net_http_res_op_error_and_reconnect;
        }

        const char * expect_accept = cpe_base64_dump(net_ws_protocol_tmp_buffer(ws_protocol), sha1_value.digest, sizeof(sha1_value.digest));
        if (strcmp(value, expect_accept) != 0) {
            CPE_ERROR(
                ws_protocol->m_em, "ws: %s: handshake response: expect token %s, but %s",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)),
                expect_accept, value);
            return net_http_res_op_error_and_reconnect;
        }

        if (net_ws_endpoint_set_state(ws_ep, net_ws_state_established) != 0) {
            return net_http_res_op_error_and_reconnect;
        }
    }

    return net_http_res_op_success;
}

static net_http_res_op_result_t net_ws_endpoint_on_handshake_complete(void * ctx, net_http_req_t req, net_http_res_result_t result) {
    net_ws_endpoint_t ws_ep = ctx;
    net_ws_protocol_t ws_protocol = net_ws_endpoint_protocol(ws_ep);

    if (ws_ep->m_state != net_ws_state_established) {
        CPE_ERROR(
            ws_protocol->m_em, "ws: %s: handshake response: no sec-websocket-accept data!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(ws_protocol),
                net_http_endpoint_net_ep(ws_ep->m_http_ep)));
        return -1;
    }
    
    return net_http_res_op_success;
}

void net_ws_endpoint_url_print(write_stream_t s, net_ws_endpoint_t ws_ep) {
    stream_printf(s, net_http_endpoint_use_https(ws_ep->m_http_ep) ? "wss://" : "ws://");
    net_address_print(s, net_endpoint_remote_address(net_http_endpoint_net_ep(ws_ep->m_http_ep)));
    stream_printf(s, "%s", net_ws_endpoint_path(ws_ep));
}

const char * net_ws_endpoint_url_dump(mem_buffer_t buffer, net_ws_endpoint_t ws_ep) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ws_endpoint_url_print((write_stream_t)&stream, ws_ep);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

const char * net_ws_state_str(net_ws_state_t state) {
    switch(state) {
    case net_ws_state_init:
        return "ws-init";
    case net_ws_state_connecting:
        return "ws-connecting";
    case net_ws_state_handshake:
        return "ws-handshake";
    case net_ws_state_established:
        return "ws-established";
    case net_ws_state_error:
        return "ws-error";
    }
}
