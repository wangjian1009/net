#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/random.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ws_cli_endpoint_i.h"
#include "net_ws_utils.h"

static int net_ws_cli_endpoint_send_event(
    net_ws_cli_protocol_t protocol, net_ws_cli_endpoint_t endpoint, struct wslay_event_msg * ws_msg)
{
    if (endpoint->m_state != net_ws_cli_endpoint_state_streaming) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: msg %s: >>> curent state %s, can`t send msg!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_wslay_op_code_str(ws_msg->opcode),
            net_ws_cli_endpoint_state_str(endpoint->m_state));
        return -1;
    }
    
    if (wslay_event_queue_msg(endpoint->m_ctx, ws_msg) != 0) {
        CPE_ERROR(
            protocol->m_em,
            "ws: %s: msg %s: >>> queue msg fail!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            net_ws_wslay_op_code_str(ws_msg->opcode));
        return -1;
    }

    if (wslay_event_want_write(endpoint->m_ctx)) {
        int rv = wslay_event_send(endpoint->m_ctx);
        if (rv != 0) {
            CPE_ERROR(
                protocol->m_em, "ws: %s: msg %s: >>> send fail, rv=%d (%s)",
                net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_ws_wslay_op_code_str(ws_msg->opcode),
                rv, net_ws_wslay_err_str(rv));
            return -1;
        }
    }
    
    if (net_endpoint_protocol_debug(endpoint->m_base_endpoint) >= 2) {
        if (ws_msg->opcode == WSLAY_TEXT_FRAME) {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg %s: >>>\n%s",
                net_endpoint_dump(
                    net_ws_cli_protocol_tmp_buffer(protocol),
                    endpoint->m_base_endpoint),
                net_ws_wslay_op_code_str(ws_msg->opcode),
                (const char *)ws_msg->msg);
        }
        else {
            CPE_INFO(
                protocol->m_em, "ws: %s: msg %s: >>> %d data success",
                net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
                net_ws_wslay_op_code_str(ws_msg->opcode),
                (int)ws_msg->msg_length);
        }
    }

    return 0;
}

int net_ws_cli_endpoint_send_msg_text(net_ws_cli_endpoint_t endpoint, const char * msg) {
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    struct wslay_event_msg ws_msg = { WSLAY_TEXT_FRAME,  (const uint8_t *)msg, strlen(msg) };
    return net_ws_cli_endpoint_send_event(protocol, endpoint, &ws_msg);
}

int net_ws_endpoint_send_msg_bin(net_ws_cli_endpoint_t endpoint, const void * msg, uint32_t msg_len) {
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    struct wslay_event_msg ws_msg = { WSLAY_BINARY_FRAME,  (const uint8_t *)msg, msg_len };
    return net_ws_cli_endpoint_send_event(protocol, endpoint, &ws_msg);
}

static ssize_t net_ws_cli_endpoint_recv(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_buf_is_empty(endpoint->m_base_endpoint, net_ep_buf_read)) {
        wslay_event_set_error(endpoint->m_ctx, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }

    uint32_t size = len;
    if (net_endpoint_buf_recv(endpoint->m_base_endpoint, net_ep_buf_read, buf, &size) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        CPE_ERROR(
            protocol->m_em, "sock: ws: %s: receive failed",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
        return -1;
    }

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     <== %d data",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            size);
    }
    
    return size;
}

static ssize_t net_ws_cli_endpoint_send(
    wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_buf_append(endpoint->m_base_endpoint, net_ep_buf_write, data, len) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     ==> %d data",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)len);
    }
    
    return len;
}

static int net_ws_cli_endpoint_genmask(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));
    
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), buf, len);

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     gen random %d data",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)len);
    }
    
    return 0;
}

static void net_ws_cli_endpoint_recv_start(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     recv begin: fin=%d, rsv=%d, opcode=%s, payload-length=" FMT_UINT64_T,
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            arg->fin, arg->rsv, net_ws_wslay_op_code_str(arg->opcode), arg->payload_length);
    }
}

static void net_ws_cli_endpoint_recv_trunk(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     recv trunk, length=%d",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            (int)arg->data_length);
    }
}

static void net_ws_cli_endpoint_recv_end(
    wslay_event_context_ptr ctx, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (net_endpoint_driver_debug(endpoint->m_base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     recv end",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint));
    }
}

static void net_ws_cli_endpoint_on_msg_recv(
    wslay_event_context_ptr ctx,  const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    net_ws_cli_endpoint_t endpoint = user_data;
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    switch(arg->opcode) {
    case WSLAY_CONTINUATION_FRAME:
        break;
    case WSLAY_TEXT_FRAME:
        break;
    case WSLAY_BINARY_FRAME:
        break;
    case WSLAY_CONNECTION_CLOSE:
        break;
    case WSLAY_PING:
        break;
    case WSLAY_PONG:
        break;
    }
    
    if (!wslay_is_ctrl_frame(arg->opcode)) {
        struct wslay_event_msg msgarg = { arg->opcode, arg->msg, arg->msg_length };
        wslay_event_queue_msg(ctx, &msgarg);
    }
}

struct wslay_event_callbacks s_net_ws_cli_endpoint_callbacks = {
    net_ws_cli_endpoint_recv,
    net_ws_cli_endpoint_send,
    net_ws_cli_endpoint_genmask,
    net_ws_cli_endpoint_recv_start,
    net_ws_cli_endpoint_recv_trunk,
    net_ws_cli_endpoint_recv_end,
    net_ws_cli_endpoint_on_msg_recv
};
