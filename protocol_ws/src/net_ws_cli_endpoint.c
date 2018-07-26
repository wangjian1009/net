#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/random.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_ws_cli_endpoint_i.h"

net_ws_cli_endpoint_t
net_ws_cli_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_ws_cli_protocol_t ws_protocol, const char * url) {
    net_endpoint_t endpoint = net_endpoint_create(driver, type, net_protocol_from_data(ws_protocol));
    if (endpoint == NULL) return NULL;
    
    net_schedule_t schedule = net_endpoint_schedule(endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);

    ws_ep->m_endpoint = endpoint;
    
    ws_ep->m_url = cpe_str_mem_dup(alloc, url);
    if (ws_ep->m_url == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "net_ws_cli_endpoint_create: dup url %s fail", url);
        net_endpoint_free(endpoint);
        return NULL;
    }

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

const char * net_ws_cli_endpoint_url(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_url;
}

static ssize_t net_ws_cli_endpoint_recv_cb(
    wslay_event_context_ptr ctx, uint8_t *data, size_t len, int flags, void *user_data)
{
    net_ws_cli_endpoint_t ws_ep = (net_ws_cli_endpoint_t)user_data;

    uint32_t size = (uint32_t)len;
    if (net_endpoint_rbuf_recv(ws_ep->m_endpoint, data, &size) != 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: rbuf recv fail!", ws_ep->m_url);
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }

    if (size == 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: rbuf recv no data!", ws_ep->m_url);
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
            "ws: %s: wbuf append fail!", ws_ep->m_url);
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    
    return (size_t)len;
}

static int net_ws_cli_endpoint_genmask_cb(
    wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    net_ws_cli_endpoint_t ws_ep = (net_ws_cli_endpoint_t)user_data;

    while(len) {
        uint32_t v = cpe_rand_dft(UINT32_MAX);
        size_t copy_len = sizeof(v) > len ? len : sizeof(v);
        memcpy(buf, &v, copy_len);
        buf += copy_len;
        len -= copy_len;
    }

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
    ws_ep->m_endpoint = NULL;
    ws_ep->m_url = NULL;
    ws_ep->m_state = net_ws_cli_state_init;
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

    if (ws_ep->m_url) {
        mem_free(alloc, ws_ep->m_url);
        ws_ep->m_url = NULL;
    }

    ws_ep->m_endpoint = NULL;
}

int net_ws_cli_endpoint_input(net_endpoint_t endpoint) {
    return 0;
}

int net_ws_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    return 0;
}
