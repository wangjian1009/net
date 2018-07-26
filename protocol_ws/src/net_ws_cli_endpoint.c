#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/random.h"
#include "cpe/utils/base64.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_ws_cli_endpoint_i.h"

static void net_ws_cli_endpoint_do_connect(net_timer_t timer, void * ctx);
static void net_ws_cli_endpoint_reset(net_ws_cli_endpoint_t ws_ep);
static int net_ws_cli_endpoint_send_handshake(net_ws_cli_endpoint_t ws_ep);

net_ws_cli_endpoint_t
net_ws_cli_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_ws_cli_protocol_t ws_protocol) {
    net_endpoint_t endpoint = net_endpoint_create(driver, type, net_protocol_from_data(ws_protocol));
    if (endpoint == NULL) return NULL;
    
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);

    ws_ep->m_endpoint = endpoint;
    
    return ws_ep;
}

void net_ws_cli_endpoint_free(net_ws_cli_endpoint_t ws_ep) {
    return net_endpoint_free(ws_ep->m_endpoint);
}

net_ws_cli_endpoint_t net_ws_cli_endpoint_get(net_endpoint_t endpoint) {
    return net_endpoint_data(endpoint);
}

void * net_ws_cli_endpoint_data(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep + 1;
}

net_ws_cli_endpoint_t net_ws_cli_endpoint_from_data(void * data) {
    return ((net_ws_cli_endpoint_t)data) - 1;
}

net_ws_cli_state_t net_ws_cli_endpoint_state(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_state;
}

net_endpoint_t net_ws_cli_endpoint_net_endpoint(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_endpoint;
}

static ssize_t net_ws_cli_endpoint_recv_cb(
    wslay_event_context_ptr ctx, uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_cli_endpoint_t ws_ep = (net_ws_cli_endpoint_t)user_data;

    uint32_t size = (uint32_t)len;
    if (net_endpoint_rbuf_recv(ws_ep->m_endpoint, data, &size) != 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: rbuf recv fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint));
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    if (size == 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: rbuf recv no data!",
            net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint));
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    return (ssize_t)size;
}

static ssize_t net_ws_cli_endpoint_send_cb(
    wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_cli_endpoint_t ws_ep = (net_ws_cli_endpoint_t)user_data;

    if (net_endpoint_wbuf_append(ws_ep->m_endpoint, data, (uint32_t)len) != 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: wbuf append fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint));
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    
    return (size_t)len;
}

static int net_ws_cli_endpoint_genmask_cb(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), buf, len);
    return 0;
}

static void net_ws_cli_endpoint_on_msg_recv_cb(
    wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    if(!wslay_is_ctrl_frame(arg->opcode)) {
        struct wslay_event_msg msgarg = {
            arg->opcode, arg->msg, arg->msg_length
        };
        wslay_event_queue_msg(ctx, &msgarg);
    }
}

static struct wslay_event_callbacks s_net_ws_cli_endpoint_callbacks = {
    net_ws_cli_endpoint_recv_cb,
    net_ws_cli_endpoint_send_cb,
    net_ws_cli_endpoint_genmask_cb,
    NULL, /* on_frame_recv_start_callback */
    NULL, /* on_frame_recv_callback */
    NULL, /* on_frame_recv_end_callback */
    net_ws_cli_endpoint_on_msg_recv_cb
};

int net_ws_cli_endpoint_init(net_endpoint_t endpoint) {
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    ws_ep->m_endpoint = NULL;
    ws_ep->m_state = net_ws_cli_state_init;
    ws_ep->m_cfg_reconnect_span_ms = 3u * 1000u;

    ws_ep->m_connect_timer = net_timer_auto_create(schedule, net_ws_cli_endpoint_do_connect, ws_ep);
    if (ws_ep->m_connect_timer == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "ws: ???: init: create connect timer fail!");
        return -1;
    }
    
    wslay_event_context_client_init(&ws_ep->m_ctx, &s_net_ws_cli_endpoint_callbacks, ws_ep);
    return 0;
}

void net_ws_cli_endpoint_fini(net_endpoint_t endpoint) {
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    
    if (ws_ep->m_ctx) {
        wslay_event_context_free(ws_ep->m_ctx);
        ws_ep->m_ctx = NULL;
    }

    if (ws_ep->m_connect_timer) {
        net_timer_free(ws_ep->m_connect_timer);
        ws_ep->m_connect_timer = NULL;
    }
    
    ws_ep->m_endpoint = NULL;
}

int net_ws_cli_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}

int net_ws_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    if (net_schedule_debug(schedule)) {
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s:    %s ==> %s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint),
            net_endpoint_state_str(old_state), net_endpoint_state_str(net_endpoint_state(endpoint)));
    }
    
    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_disable:
        net_ws_cli_endpoint_reset(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        break;
    case net_endpoint_state_network_error:
        net_ws_cli_endpoint_reset(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        break;
    case net_endpoint_state_logic_error:
        net_ws_cli_endpoint_reset(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, 0);
        break;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        break;
    case net_endpoint_state_established:
        if (net_ws_cli_endpoint_send_handshake(ws_ep) != 0) {
            net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        }
        break;
    }

    return 0;
}

static void net_ws_cli_endpoint_do_connect(net_timer_t timer, void * ctx) {
    net_ws_cli_endpoint_t ws_ep = ctx;
    
    if (net_endpoint_connect(ws_ep->m_endpoint) != 0) {
        net_timer_active(timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
    }
}

static void net_ws_cli_endpoint_reset(net_ws_cli_endpoint_t ws_ep) {
}

static int net_ws_cli_endpoint_send_handshake(net_ws_cli_endpoint_t ws_ep) {
    net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
    
    uint32_t size = 1024;
    void * buf = net_endpoint_wbuf_alloc(ws_ep->m_endpoint, &size);
    if (buf == NULL) {
    }

    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, size);

    net_address_t address = net_endpoint_remote_address(ws_ep->m_endpoint);
    
    int n = stream_printf(
        (write_stream_t)&ws,
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: ",
        net_address_host(net_schedule_tmp_buffer(schedule), address),
        net_address_port(address));

    char rand_buf[16];
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), rand_buf, sizeof(rand_buf));
    struct read_stream_mem rand_buf_is = CPE_READ_STREAM_MEM_INITIALIZER(rand_buf, sizeof(rand_buf));
    n += cpe_base64_encode((write_stream_t)&ws, (read_stream_t)&rand_buf_is);
    
    n += stream_printf(
        (write_stream_t)&ws,
        "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n");

    if (net_endpoint_wbuf_supply(ws_ep->m_endpoint, (uint32_t)n) != 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "ws: %s:    handshake: write fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
        return -1;
    }
    
    return 0;
}
