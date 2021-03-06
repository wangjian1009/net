#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/random.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ws_endpoint_i.h"
#include "net_ws_stream_endpoint_i.h"
#include "net_ws_utils.h"

static int net_ws_endpoint_flush(net_ws_protocol_t protocol, net_ws_endpoint_t endpoint);
static int net_ws_endpoint_send_event(
    net_ws_protocol_t protocol, net_ws_endpoint_t endpoint, struct wslay_event_msg * ws_msg);

static int net_ws_endpoint_dispatch_close(
    net_ws_protocol_t protocol, net_ws_endpoint_t endpoint,
    uint16_t status_code, void const * msg, uint32_t msg_len);

int net_ws_endpoint_send_msg_text(net_ws_endpoint_t endpoint, const char * msg) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    struct wslay_event_msg ws_msg = { WSLAY_TEXT_FRAME,  (const uint8_t *)msg, strlen(msg) };
    return net_ws_endpoint_send_event(protocol, endpoint, &ws_msg);
}

int net_ws_endpoint_send_msg_bin(net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    struct wslay_event_msg ws_msg = { WSLAY_BINARY_FRAME,  (const uint8_t *)msg, msg_len };
    return net_ws_endpoint_send_event(protocol, endpoint, &ws_msg);
}

int net_ws_endpoint_close(net_ws_endpoint_t endpoint, uint16_t status_code, const void * msg, uint32_t msg_len) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (!net_endpoint_is_writeable(endpoint->m_base_endpoint)) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: can`t close in state %s",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_endpoint_state_str(net_endpoint_state(endpoint->m_base_endpoint)));
        return -1;
    }

    if (endpoint->m_state != net_ws_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: can`t close in state %s",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_endpoint_state_str(endpoint->m_state));
        return -1;
    }
    
    assert(endpoint->m_ws_ctx);
    int rv = wslay_event_queue_close(endpoint->m_ws_ctx, status_code, msg, msg_len);
    if (rv != 0) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: close %d: >>> queue close fail, %d(%s)!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            status_code, rv, net_ws_wslay_err_str(rv));
        return -1;
    }

    if (net_ws_endpoint_flush(protocol, endpoint) != 0) return -1;

    return net_ws_endpoint_dispatch_close(protocol, endpoint, status_code, msg, msg_len);
}

static ssize_t net_ws_endpoint_recv(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_buf_is_empty(endpoint->m_base_endpoint, net_ep_buf_read)) {
        wslay_event_set_error(endpoint->m_ws_ctx, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }

    uint32_t size = len;
    if (net_endpoint_buf_recv(endpoint->m_base_endpoint, net_ep_buf_read, buf, &size) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        CPE_ERROR(
            protocol->m_em, "ws: %s: receive failed",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     <== %d data",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            size);
    }
    
    return size;
}

static ssize_t net_ws_endpoint_send(
    wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_buf_append(endpoint->m_base_endpoint, net_ep_buf_write, data, len) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     ==> %d data",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)len);
    }
    
    return len;
}

static int net_ws_endpoint_genmask(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), buf, len);

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     gen random %d data",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)len);
    }
    
    return 0;
}

static void net_ws_endpoint_recv_start(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     recv begin: fin=%d, rsv=%d, opcode=%s, payload-length=" FMT_UINT64_T,
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            arg->fin, arg->rsv, net_ws_wslay_op_code_str(arg->opcode), arg->payload_length);
    }
}

static void net_ws_endpoint_recv_trunk(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     recv trunk, length=%d",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)arg->data_length);
    }
}

static void net_ws_endpoint_recv_end(
    wslay_event_context_ptr ctx, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "ws: %s:     recv end",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
    }
}

static void net_ws_endpoint_on_msg_recv(
    wslay_event_context_ptr ctx,  const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    net_ws_endpoint_t endpoint = user_data;
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    switch(arg->opcode) {
    case WSLAY_CONTINUATION_FRAME:
        CPE_ERROR(
            protocol->m_em, "ws: %s: continuation: not support!",
            net_endpoint_dump(
                net_ws_protocol_tmp_buffer(protocol),
                endpoint->m_base_endpoint));
        break;
    case WSLAY_TEXT_FRAME: {
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg text: <<< %.*s",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                (int)arg->msg_length, (const char *)arg->msg);
        }

        if (endpoint->m_on_msg_text_fun) {
            mem_buffer_clear_data(&protocol->m_data_buffer);
            char * buf = mem_buffer_alloc(&protocol->m_data_buffer, arg->msg_length + 1);
            if (buf == NULL) {
                CPE_ERROR(
                    protocol->m_em, "ws: %s: msg text: alloc buf fail, msg-length=%d!",
                    net_endpoint_dump(
                        net_ws_protocol_tmp_buffer(protocol),
                        endpoint->m_base_endpoint),
                    (int)arg->msg_length);
                break;
            }
            memcpy(buf, arg->msg, arg->msg_length);
            buf[arg->msg_length] = 0;

            endpoint->m_on_msg_text_fun(
                endpoint->m_ctx, endpoint,
                mem_buffer_make_continuous(&protocol->m_data_buffer, 0));
        }
        break;
    }
    case WSLAY_BINARY_FRAME:
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg bin: <<< %d data",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                (int)arg->msg_length);
        }

        if (endpoint->m_stream) {
            net_endpoint_t base_stream = net_endpoint_from_data(endpoint->m_stream);
            if (net_endpoint_buf_append(base_stream, net_ep_buf_read, arg->msg, (uint32_t)arg->msg_length) != 0) {
                if (net_endpoint_is_active(base_stream)) {
                    if (!net_endpoint_have_error(base_stream)) {
                        net_endpoint_set_error(
                            base_stream, net_endpoint_error_source_network, net_endpoint_network_errno_internal, "WsForwardError");
                    }
                    if (net_endpoint_set_state(base_stream, net_endpoint_state_error) != 0) {
                        net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
                    }
                }
            }
        }

        if (endpoint->m_base_endpoint
            && net_endpoint_is_active(endpoint->m_base_endpoint)
            && endpoint->m_on_msg_bin_fun)
        {
            endpoint->m_on_msg_bin_fun(
                endpoint->m_ctx, endpoint, arg->msg, (uint32_t)arg->msg_length);
        }
        
        break;
    case WSLAY_CONNECTION_CLOSE:
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ws: %s: close <<< status=%d, %d data",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                arg->status_code, (int)arg->msg_length - 2);
        }

        if (net_ws_endpoint_dispatch_close(
                protocol, endpoint, arg->status_code, ((uint8_t*)arg->msg) + 2, arg->msg_length - 2)
            != 0)
        {
            net_endpoint_set_state(endpoint->m_base_endpoint, net_endpoint_state_deleting);
            return;
        }

        break;
    case WSLAY_PING:
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ws: %s: ping <<< %d data",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                (int)arg->msg_length);
        }
        break;
    case WSLAY_PONG:
        if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg pong: <<< %d data",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                (int)arg->msg_length);
        }
        
        break;
    }
}

struct wslay_event_callbacks s_net_ws_endpoint_callbacks = {
    net_ws_endpoint_recv,
    net_ws_endpoint_send,
    net_ws_endpoint_genmask,
    net_ws_endpoint_recv_start,
    net_ws_endpoint_recv_trunk,
    net_ws_endpoint_recv_end,
    net_ws_endpoint_on_msg_recv
};

static int net_ws_endpoint_flush(net_ws_protocol_t protocol, net_ws_endpoint_t endpoint) {
    if (wslay_event_want_write(endpoint->m_ws_ctx)) {
        uint8_t ws_ctx_processing_tag_local = 0;
        if (!endpoint->m_ws_ctx_is_processing) {
            ws_ctx_processing_tag_local = 1;
            endpoint->m_ws_ctx_is_processing = 1;
        }

        int rv = wslay_event_send(endpoint->m_ws_ctx);

        if (ws_ctx_processing_tag_local) {
            endpoint->m_ws_ctx_is_processing = 0;
            if (endpoint->m_ws_ctx_is_free) {
                net_ws_endpoint_free_ws_ctx(endpoint);
            }
        }

        if (net_endpoint_state(endpoint->m_base_endpoint) == net_endpoint_state_deleting) return -1;
        
        if (rv != 0) {
            CPE_ERROR(
                protocol->m_em, "ws: %s: send fail, rv=%d (%s)",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                rv, net_ws_wslay_err_str(rv));
            return -1;
        }
    }

    return 0;
}

static int net_ws_endpoint_send_event(
    net_ws_protocol_t protocol, net_ws_endpoint_t endpoint, struct wslay_event_msg * ws_msg)
{
    if (endpoint->m_state != net_ws_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: msg %s: >>> curent state %s, can`t send msg!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_wslay_op_code_str(ws_msg->opcode),
            net_ws_endpoint_state_str(endpoint->m_state));
        return -1;
    }
    
    if (wslay_event_queue_msg(endpoint->m_ws_ctx, ws_msg) != 0) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: msg %s: >>> queue msg fail!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_wslay_op_code_str(ws_msg->opcode));
        return -1;
    }

    if (net_ws_endpoint_flush(protocol, endpoint) != 0) return -1;
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        if (ws_msg->opcode == WSLAY_TEXT_FRAME) {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg %s: >>>\n%s",
                net_endpoint_dump(
                    net_ws_protocol_tmp_buffer(protocol),
                    endpoint->m_base_endpoint),
                net_ws_wslay_op_code_str(ws_msg->opcode),
                (const char *)ws_msg->msg);
        }
        else {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg %s: >>> %d data success",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_ws_wslay_op_code_str(ws_msg->opcode),
                (int)ws_msg->msg_length);
        }
    }

    return 0;
}

static int net_ws_endpoint_update_state_by_status(
    net_ws_protocol_t protocol, net_endpoint_t base_endpoint,
    uint16_t status_code, void const * msg, uint32_t msg_len)
{
    if (status_code == WSLAY_CODE_NORMAL_CLOSURE) {
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_disable);
    } else {
        char code_buf[16];
        if (msg_len == 0) {
            net_endpoint_set_error(
                base_endpoint, net_endpoint_error_source_websocket, status_code,
                net_ws_status_code_str(code_buf, sizeof(code_buf), status_code));
        }
        else {
            if (cpe_str_validate_utf8((const char *)msg, msg_len)) {
                char buf[512];
                snprintf(
                    buf, sizeof(buf), "%s(%.*s)",
                    net_ws_status_code_str(code_buf, sizeof(code_buf), status_code),
                    msg_len, (const char *)msg);
                buf[sizeof(buf) - 1] = 0;
                
                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_websocket, status_code, buf);
            }
            else {
                char buf[64];
                snprintf(
                    buf, sizeof(buf), "%s(%d msg)",
                    net_ws_status_code_str(code_buf, sizeof(code_buf), status_code), msg_len);

                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_websocket, status_code, buf);
            }
        }
        
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_error);
    }
}

static int net_ws_endpoint_dispatch_close(
    net_ws_protocol_t protocol, net_ws_endpoint_t endpoint,
    uint16_t status_code, void const * msg, uint32_t msg_len)
{
    if (endpoint->m_stream) {
        net_ws_stream_endpoint_t stream = endpoint->m_stream;
        stream->m_underline = NULL;
        endpoint->m_stream = NULL;

        net_endpoint_t base_stream = net_endpoint_from_data(stream);
        if (net_endpoint_is_active(base_stream)) {
            if (net_ws_endpoint_update_state_by_status(protocol, base_stream, status_code, msg, msg_len) != 0) {
                net_endpoint_set_state(base_stream, net_endpoint_state_deleting);
            } 
        }
    }

    if (endpoint->m_on_close_fun) {
        endpoint->m_on_close_fun(endpoint->m_ctx, endpoint, status_code, msg, msg_len);
    }

    if (net_endpoint_is_writeable(endpoint->m_base_endpoint)) {
        return net_ws_endpoint_update_state_by_status(protocol, endpoint->m_base_endpoint, status_code, msg, msg_len);
    }
    else {
        return 0;
    }
}
