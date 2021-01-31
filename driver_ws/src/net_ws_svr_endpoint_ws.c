#include <assert.h>
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/random.h"
#include "net_protocol.h"
#include "net_driver.h"
#include "net_endpoint.h"
#include "net_ws_svr_endpoint_i.h"
#include "net_ws_utils.h"

static ssize_t net_ws_svr_endpoint_recv(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data)
{
    net_endpoint_t base_endpoint = user_data;
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_read)) {
        wslay_event_set_error(endpoint->m_ctx, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }

    uint32_t size = len;
    if (net_endpoint_buf_recv(base_endpoint, net_ep_buf_read, buf, &size) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        CPE_ERROR(
            protocol->m_em, "sock: ws: %s: receive failed",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     <== %d data",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
            size);
    }
    
    return size;
}

static ssize_t net_ws_svr_endpoint_send(
    wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    net_endpoint_t base_endpoint = user_data;
    net_ws_svr_endpoint_t endpoint = net_endpoint_protocol_data(base_endpoint);
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_buf_append(base_endpoint, net_ep_buf_write, data, len) != 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     ==> %d data",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
            (int)len);
    }
    
    return len;
}

static int net_ws_svr_endpoint_genmask(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    net_endpoint_t base_endpoint = user_data;
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));
    
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), buf, len);

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     gen random %d data",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
            (int)len);
    }
    
    return 0;
}

static void net_ws_svr_endpoint_recv_start(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data)
{
    net_endpoint_t base_endpoint = user_data;
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            protocol->m_em, "sock: ws: %s:     recv start",
            net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint));
    }
}

static void net_ws_svr_endpoint_recv_trunk(
    wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data)
{
}

static void net_ws_svr_endpoint_recv_end(
    wslay_event_context_ptr ctx, void *user_data)
{
}

static void net_ws_svr_endpoint_on_msg_recv(
    wslay_event_context_ptr ctx,  const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    /* if (!wslay_is_ctrl_frame(arg->opcode)) { */
    /*     struct wslay_event_msg msgarg = { arg->opcode, arg->msg, arg->msg_length }; */
    /*     wslay_event_queue_msg(ctx, &msgarg); */
    /* } */
}

struct wslay_event_callbacks s_net_ws_svr_endpoint_callbacks = {
    net_ws_svr_endpoint_recv,
    net_ws_svr_endpoint_send,
    net_ws_svr_endpoint_genmask,
    net_ws_svr_endpoint_recv_start,
    net_ws_svr_endpoint_recv_trunk,
    net_ws_svr_endpoint_recv_end,
    net_ws_svr_endpoint_on_msg_recv
};
