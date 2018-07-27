#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/random.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/sha1.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_ws_cli_endpoint_i.h"
#include "net_ws_cli_protocol_i.h"

static void net_ws_cli_endpoint_do_connect(net_timer_t timer, void * ctx);
static void net_ws_cli_endpoint_do_process(net_timer_t timer, void * ctx);
static void net_ws_cli_endpoint_reset_data(net_ws_cli_endpoint_t ws_ep);
static int net_ws_cli_endpoint_send_handshake(net_ws_cli_endpoint_t ws_ep);
static int net_ws_cli_endpoint_on_handshake(net_schedule_t schedule, net_ws_cli_endpoint_t ws_ep, char * response);
static int net_ws_cli_endpoint_notify_state_changed(net_ws_cli_endpoint_t ws_ep, net_ws_cli_state_t old_state);

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

uint8_t net_ws_cli_endpoint_debug(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_debug;
}

void net_ws_cli_endpoint_set_debug(net_ws_cli_endpoint_t ws_ep, uint8_t debug) {
    ws_ep->m_debug = debug;
}

const char * net_ws_cli_endpoint_path(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_cfg_path ? ws_ep->m_cfg_path : "/";
}

int net_ws_cli_endpoint_set_path(net_ws_cli_endpoint_t ws_ep, const char * path) {
    net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);

    if (ws_ep->m_cfg_path) {
        mem_free(alloc, ws_ep->m_cfg_path);
        ws_ep->m_cfg_path = NULL;
    }

    if (path) {
        ws_ep->m_cfg_path = cpe_str_mem_dup(alloc, path);
        if (ws_ep->m_cfg_path == NULL) {
            CPE_ERROR(
                net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
                "ws: %s: set path: copy path %s fail!",
                net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint), path);
            return -1;
        }
    }
    
    return 0;
}

uint32_t net_ws_cli_endpoint_reconnect_span_ms(net_ws_cli_endpoint_t ws_ep) {
    return ws_ep->m_cfg_reconnect_span_ms;
}

void net_ws_cli_endpoint_set_reconnect_span_ms(net_ws_cli_endpoint_t ws_ep, uint32_t span_ms) {
    ws_ep->m_cfg_reconnect_span_ms = span_ms;
}

int net_ws_cli_endpoint_send_msg_text(net_ws_cli_endpoint_t ws_ep, const char * msg) {
    struct wslay_event_msg ws_msg = { WSLAY_TEXT_FRAME,  (const uint8_t *)msg, strlen(msg) };

    if (wslay_event_queue_msg(ws_ep->m_ctx, &ws_msg) != 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: send msg text: queue msg fail!",
            net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint));
        return -1;
    }

    if (ws_ep->m_debug >= 2) {
        net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s: handshake request: >>>\n%s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            msg);
    }

    net_timer_active(ws_ep->m_process_timer, 0);
    
    return 0;
}

int net_ws_cli_endpoint_send_msg_bin(net_ws_cli_endpoint_t ws_ep, const void * msg, uint32_t msg_len) {
    struct wslay_event_msg ws_msg = { WSLAY_BINARY_FRAME,  (const uint8_t *)msg, msg_len };

    if (wslay_event_queue_msg(ws_ep->m_ctx, &ws_msg) != 0) {
        CPE_ERROR(
            net_schedule_em(net_endpoint_schedule(ws_ep->m_endpoint)),
            "ws: %s: send msg text: queue bintray fail, len=%d!",
            net_endpoint_dump(net_schedule_tmp_buffer(net_endpoint_schedule(ws_ep->m_endpoint)), ws_ep->m_endpoint), msg_len);
        return -1;
    }

    if (ws_ep->m_debug >= 2) {
        net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s: handshake request: >>> %d data",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            msg_len);
    }

    net_timer_active(ws_ep->m_process_timer, 0);
        
    return 0;
}

void net_ws_cli_endpoint_enable(net_ws_cli_endpoint_t ws_ep) {
    net_timer_active(ws_ep->m_connect_timer, 0);
}

int net_ws_cli_endpoint_set_state(net_ws_cli_endpoint_t ws_ep, net_ws_cli_state_t state) {
    if (ws_ep->m_state == state) return 0;
    
    if (ws_ep->m_debug >= 1) {
        net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s: state %s ==> %s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            net_ws_cli_state_str(ws_ep->m_state),
            net_ws_cli_state_str(state));
    }

    net_ws_cli_state_t old_state = ws_ep->m_state;
    
    ws_ep->m_state = state;

    return net_ws_cli_endpoint_notify_state_changed(ws_ep, old_state);
}

static int net_ws_cli_endpoint_notify_state_changed(net_ws_cli_endpoint_t ws_ep, net_ws_cli_state_t old_state) {
    net_ws_cli_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(ws_ep->m_endpoint));
        
    int rv = ws_protocol->m_endpoint_on_state_change
        ? ws_protocol->m_endpoint_on_state_change(ws_ep, old_state)
        : 0;
    
    /* net_endpoint_monitor_t monitor = TAILQ_FIRST(&endpoint->m_monitors); */
    /* while(monitor) { */
    /*     net_endpoint_monitor_t next_monitor = TAILQ_NEXT(monitor, m_next); */
    /*     assert(!monitor->m_is_free); */
    /*     assert(!monitor->m_is_processing); */

    /*     if (monitor->m_on_state_change) { */
    /*         monitor->m_is_processing = 1; */
    /*         monitor->m_on_state_change(monitor->m_ctx, endpoint, old_state); */
    /*         monitor->m_is_processing = 0; */
    /*         if (monitor->m_is_free) net_endpoint_monitor_free(monitor); */
    /*     } */
        
    /*     monitor = next_monitor; */
    /* } */

    return rv;
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
    ws_ep->m_debug = net_schedule_debug(schedule);
    ws_ep->m_cfg_reconnect_span_ms = 3u * 1000u;
    ws_ep->m_cfg_path = NULL;
    ws_ep->m_handshake_token = NULL;

    ws_ep->m_connect_timer = net_timer_auto_create(schedule, net_ws_cli_endpoint_do_connect, ws_ep);
    if (ws_ep->m_connect_timer == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "ws: ???: init: create connect timer fail!");
        return -1;
    }
    
    ws_ep->m_process_timer = net_timer_auto_create(schedule, net_ws_cli_endpoint_do_process, ws_ep);
    if (ws_ep->m_process_timer == NULL) {
        CPE_ERROR(net_schedule_em(schedule), "ws: ???: init: create process timer fail!");
        net_timer_free(ws_ep->m_connect_timer);
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

    if (ws_ep->m_process_timer) {
        net_timer_free(ws_ep->m_process_timer);
        ws_ep->m_process_timer = NULL;
    }

    if (ws_ep->m_handshake_token) {
        mem_free(alloc, ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    if (ws_ep->m_cfg_path) {
        mem_free(alloc, ws_ep->m_cfg_path);
        ws_ep->m_cfg_path = NULL;
    }
    
    ws_ep->m_endpoint = NULL;
}

int net_ws_cli_endpoint_input(net_endpoint_t endpoint) {
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    if (ws_ep->m_state == net_ws_cli_state_handshake) {
        char * data;
        uint32_t size;
        if (net_endpoint_rbuf_by_str(endpoint, "\r\n\r\n", (void**)&data, &size) != 0) {
            CPE_ERROR(
                net_schedule_em(schedule), "ws: %s: handshake response: search sep fail",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint));
            return -1;
        }

        if (data == NULL) {
            if(net_endpoint_rbuf_size(endpoint) > 8192) {
                CPE_ERROR(
                    net_schedule_em(schedule), "ws: %s: handshake response: Too big response head!, size=%d",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), endpoint),
                    net_endpoint_rbuf_size(endpoint));
                return -1;
            }
            else {
                return 0;
            }
        }

        if (net_ws_cli_endpoint_on_handshake(schedule, ws_ep, data) != 0) return -1;

        net_endpoint_rbuf_consume(endpoint, size);
    }

    if (ws_ep->m_state == net_ws_cli_state_established) {
        net_timer_active(ws_ep->m_process_timer, 0);
    }
    
    return ws_ep->m_state == net_ws_cli_state_established ? 0 : -1;
}

int net_ws_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_ws_cli_endpoint_t ws_ep = net_endpoint_protocol_data(endpoint);
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_disable:
        if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_init) != 0) return -1;
        net_ws_cli_endpoint_reset_data(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        break;
    case net_endpoint_state_network_error:
        if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_error) != 0) return -1;
        net_ws_cli_endpoint_reset_data(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        break;
    case net_endpoint_state_logic_error:
        if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_error) != 0) return -1;
        net_ws_cli_endpoint_reset_data(ws_ep);
        net_timer_active(ws_ep->m_connect_timer, 0);
        break;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_ws_cli_endpoint_send_handshake(ws_ep) != 0) {
            if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_error) != 0) return -1;
            net_timer_active(ws_ep->m_connect_timer, (int32_t)ws_ep->m_cfg_reconnect_span_ms);
        }
        else {
            if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_handshake) != 0) return -1;
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

static void net_ws_cli_endpoint_do_process(net_timer_t timer, void * ctx) {
    net_ws_cli_endpoint_t ws_ep = ctx;

    if (wslay_event_want_read(ws_ep->m_ctx) && !net_endpoint_rbuf_is_empty(ws_ep->m_endpoint)) {
        if (wslay_event_recv(ws_ep->m_ctx) != 0) {
            net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
            CPE_ERROR(
                net_schedule_em(schedule), "ws: %s: process: recv fail, auto disconnect",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
            net_endpoint_set_state(ws_ep->m_endpoint, net_endpoint_state_disable);
            return;
        }
    }

    if (wslay_event_want_write(ws_ep->m_ctx) && !net_endpoint_wbuf_is_full(ws_ep->m_endpoint)) {
        if (wslay_event_send(ws_ep->m_ctx) != 0) {
            net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
            CPE_ERROR(
                net_schedule_em(schedule), "ws: %s: process: send fail, auto disconnect",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
            net_endpoint_set_state(ws_ep->m_endpoint, net_endpoint_state_disable);
            return;
        }
    }
}

static void net_ws_cli_endpoint_reset_data(net_ws_cli_endpoint_t ws_ep) {
    net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    
    if (ws_ep->m_handshake_token) {
        mem_free(alloc, ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    wslay_event_context_free(ws_ep->m_ctx);
    wslay_event_context_client_init(&ws_ep->m_ctx, &s_net_ws_cli_endpoint_callbacks, ws_ep);
}

static int net_ws_cli_endpoint_send_handshake(net_ws_cli_endpoint_t ws_ep) {
    net_schedule_t schedule = net_endpoint_schedule(ws_ep->m_endpoint);

    /*生成token */
    if (ws_ep->m_handshake_token) {
        mem_free(net_schedule_allocrator(schedule), ws_ep->m_handshake_token);
        ws_ep->m_handshake_token = NULL;
    }

    char rand_buf[16];
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), rand_buf, sizeof(rand_buf));
    ws_ep->m_handshake_token =
        cpe_str_mem_dup(
            net_schedule_allocrator(schedule), 
            cpe_base64_dump(net_schedule_tmp_buffer(schedule), rand_buf, sizeof(rand_buf)));
    if (ws_ep->m_handshake_token == NULL) {
        CPE_ERROR(
            net_schedule_em(schedule), "ws: %s:    handshake: generate token fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
        return -1;
    }
            
    uint32_t size = 2048;
    void * buf = net_endpoint_wbuf_alloc(ws_ep->m_endpoint, &size);
    if (buf == NULL) {
        CPE_ERROR(
            net_schedule_em(schedule), "ws: %s:    handshake: alloc buf fail, size=%d",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            (int)size);
        return -1;
    }

    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, size);

    net_address_t address = net_endpoint_remote_address(ws_ep->m_endpoint);

    int n = stream_printf(
        (write_stream_t)&ws,
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        net_ws_cli_endpoint_path(ws_ep),
        net_address_host(net_schedule_tmp_buffer(schedule), address),
        net_address_port(address),
        ws_ep->m_handshake_token);

    if (ws_ep->m_debug >= 2) {
        ((char*)buf)[n] = 0;
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s: handshake request: >>>\n%s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            buf);
    }
    
    if (net_endpoint_wbuf_supply(ws_ep->m_endpoint, (uint32_t)n) != 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "ws: %s: handshake: write fail",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
        return -1;
    }
    
    return 0;
}

static int net_ws_cli_endpoint_on_handshake_line(net_schedule_t schedule, net_ws_cli_endpoint_t ws_ep, char * line) {
    char * sep = strchr(line, ':');
    if (sep == NULL) return 0;

    char * name = line;
    char * value = cpe_str_trim_head(sep + 1);

    *cpe_str_trim_tail(sep, line) = 0;
    
    if (strcasecmp(name, "sec-websocket-accept") == 0) {
        char accept_token_buf[64];
        snprintf(accept_token_buf, sizeof(accept_token_buf), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_ep->m_handshake_token);

        struct cpe_sha1_value sha1_value;
        if (cpe_sha1_encode_str(&sha1_value, accept_token_buf) != 0) {
            CPE_ERROR(
                net_schedule_em(schedule), "ws: %s: handshake response: calc accept token fail",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
            return -1;
        }

        const char * expect_accept = cpe_base64_dump(net_schedule_tmp_buffer(schedule), sha1_value.digest, sizeof(sha1_value.digest));
        if (strcmp(value, expect_accept) != 0) {
            CPE_ERROR(
                net_schedule_em(schedule), "ws: %s: handshake response: expect token %s, but %s",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
                expect_accept, value);
            return -1;
        }

        if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_established) != 0) return -1;
    }
    
    return 0;
}

static int net_ws_cli_endpoint_on_handshake(net_schedule_t schedule, net_ws_cli_endpoint_t ws_ep, char * response) {
    if (ws_ep->m_debug >= 2) {
        CPE_INFO(
            net_schedule_em(schedule), "ws: %s: handshake response: <<<\n%s",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint),
            response);
    }

    char * line = response;
    char * sep = strstr(line, "\r\n");
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_ws_cli_endpoint_on_handshake_line(schedule, ws_ep, line) != 0) return -1;
    }

    if (line[0]) {
        if (net_ws_cli_endpoint_on_handshake_line(schedule, ws_ep, line) != 0) return -1;
    }

    if (ws_ep->m_state != net_ws_cli_state_established) {
        CPE_ERROR(
            net_schedule_em(schedule), "ws: %s: handshake response: no sec-websocket-accept data!",
            net_endpoint_dump(net_schedule_tmp_buffer(schedule), ws_ep->m_endpoint));
        return -1;
    }
    
    return 0;
}

const char * net_ws_cli_state_str(net_ws_cli_state_t state) {
    switch(state) {
    case net_ws_cli_state_init:
        return "ws-init";
    case net_ws_cli_state_connecting:
        return "ws-connecting";
    case net_ws_cli_state_handshake:
        return "ws-handshake";
    case net_ws_cli_state_established:
        return "ws-established";
    case net_ws_cli_state_error:
        return "ws-error";
    }
}
