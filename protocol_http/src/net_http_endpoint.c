#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/random.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/sha1.h"
#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_http_endpoint_i.h"
#include "net_http_protocol_i.h"
#include "net_http_req_i.h"

static void net_http_endpoint_reset_data(net_http_endpoint_t http_ep);
static void net_http_endpoint_do_connect(net_timer_t timer, void * ctx);
static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state);

net_http_endpoint_t
net_http_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_http_protocol_t ws_protocol) {
    net_endpoint_t endpoint = net_endpoint_create(driver, type, net_protocol_from_data(ws_protocol));
    if (endpoint == NULL) return NULL;
    
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

    return http_ep;
}

void net_http_endpoint_free(net_http_endpoint_t http_ep) {
    return net_endpoint_free(http_ep->m_endpoint);
}

net_http_endpoint_t net_http_endpoint_get(net_endpoint_t endpoint) {
    return net_endpoint_data(endpoint);
}

void * net_http_endpoint_data(net_http_endpoint_t http_ep) {
    return http_ep + 1;
}

net_http_endpoint_t net_http_endpoint_from_data(void * data) {
    return ((net_http_endpoint_t)data) - 1;
}

net_http_state_t net_http_endpoint_state(net_http_endpoint_t http_ep) {
    return http_ep->m_state;
}

net_schedule_t net_http_endpoint_schedule(net_http_endpoint_t http_ep) {
    return net_endpoint_schedule(http_ep->m_endpoint);
}

net_endpoint_t net_http_endpoint_net_ep(net_http_endpoint_t http_ep) {
    return http_ep->m_endpoint;
}

uint32_t net_http_endpoint_reconnect_span_ms(net_http_endpoint_t http_ep) {
    return http_ep->m_reconnect_span_ms;
}

void net_http_endpoint_set_reconnect_span_ms(net_http_endpoint_t http_ep, uint32_t span_ms) {
    http_ep->m_reconnect_span_ms = span_ms;
}

net_http_protocol_t net_http_endpoint_protocol(net_http_endpoint_t http_ep) {
    return net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));
}

uint8_t net_http_endpoint_use_https(net_http_endpoint_t http_ep) {
    return http_ep->m_use_https;
}

void net_http_endpoint_set_use_https(net_http_endpoint_t http_ep, uint8_t use_https) {
    http_ep->m_use_https = 0;
}

uint8_t net_http_endpoint_keep_alive(net_http_endpoint_t http_ep) {
    return http_ep->m_keep_alive;
}

void net_http_endpoint_set_kee_alive(net_http_endpoint_t http_ep, uint8_t keep_alive) {
    http_ep->m_keep_alive = keep_alive;
}

void net_http_endpoint_upgrade_connection(net_http_endpoint_t http_ep, net_http_endpoint_input_fun_t input_processor) {
    http_ep->m_upgraded_processor = input_processor;
}

int net_http_endpoint_set_remote(net_http_endpoint_t http_ep, const char * url) {
    net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));

    if (cpe_str_start_with(url, "http://")) {
        http_ep->m_use_https = 0;
    }
    else if (cpe_str_start_with(url, "https://")) {
        http_ep->m_use_https = 1;
    }
    else {
        CPE_ERROR(
            ws_protocol->m_em,
            "http: %s: set remote and path: url %s check protocol fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint), url);
        return -1;
    }

    const char * addr_begin = url + 5; /*http://*/
    const char * addr_end = strchr(addr_begin, '/');

    const char * str_address = NULL;
    const char * path = NULL;
    
    if (addr_end) {
        mem_buffer_t tmp_buffer = net_http_protocol_tmp_buffer(ws_protocol);
        mem_buffer_clear_data(tmp_buffer);
        str_address = mem_buffer_strdup_range(tmp_buffer, addr_begin, addr_end);
        if (str_address == NULL) {
            CPE_ERROR(
                ws_protocol->m_em,
                "http: %s: set remote and path: url %s address dup fail!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint), url);
            return -1;
        }
        path = addr_end;
    }
    else {
        str_address = addr_begin;
    }

    net_address_t address = net_address_create_auto(net_endpoint_schedule(http_ep->m_endpoint), addr_begin);
    if (address == NULL) {
        CPE_ERROR(
            ws_protocol->m_em,
            "http: %s: set remote and path: url %s address format error!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint), url);
        return -1;
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, http_ep->m_use_https ? 443 : 80);
    }
        
    if (net_endpoint_set_remote_address(http_ep->m_endpoint, address, 1) != 0) {
        CPE_ERROR(
            ws_protocol->m_em,
            "http: %s: set remote and path: url %s set remote address fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint), url);
        net_address_free(address);
        return -1;
    }

    return 0;
}

void net_http_endpoint_enable(net_http_endpoint_t http_ep) {
    net_timer_active(http_ep->m_connect_timer, 0);
}

int net_http_endpoint_set_state(net_http_endpoint_t http_ep, net_http_state_t state) {
    if (http_ep->m_state == state) return 0;
    
    if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 1) {
        net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));
        CPE_INFO(
            ws_protocol->m_em, "http: %s: state %s ==> %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint),
            net_http_state_str(http_ep->m_state),
            net_http_state_str(state));
    }

    net_http_state_t old_state = http_ep->m_state;
    
    http_ep->m_state = state;

    return net_http_endpoint_notify_state_changed(http_ep, old_state);
}

static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state) {
    net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));
        
    int rv = ws_protocol->m_endpoint_on_state_change
        ? ws_protocol->m_endpoint_on_state_change(http_ep, old_state)
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

int net_http_endpoint_init(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    http_ep->m_endpoint = endpoint;
    http_ep->m_state = net_http_state_disable;
    http_ep->m_use_https = 0;
    http_ep->m_reconnect_span_ms = 3u * 1000u;
    http_ep->m_max_req_id = 0;
    TAILQ_INIT(&http_ep->m_runing_reqs);
    TAILQ_INIT(&http_ep->m_completed_reqs);
    http_ep->m_upgraded_processor = NULL;
    
    http_ep->m_connect_timer = net_timer_auto_create(schedule, net_http_endpoint_do_connect, http_ep);
    if (http_ep->m_connect_timer == NULL) {
        CPE_ERROR(ws_protocol->m_em, "http: ???: init: create connect timer fail!");
        return -1;
    }
    
    if (ws_protocol->m_endpoint_init && ws_protocol->m_endpoint_init(http_ep) != 0) {
        CPE_ERROR(ws_protocol->m_em, "http: ???: init: external init fail!");
        net_timer_free(http_ep->m_connect_timer);
        return -1;
    }
    
    return 0;
}

void net_http_endpoint_fini(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    while(!TAILQ_EMPTY(&http_ep->m_runing_reqs)) {
        net_http_req_free(TAILQ_FIRST(&http_ep->m_runing_reqs));
    }
    
    while(!TAILQ_EMPTY(&http_ep->m_completed_reqs)) {
        net_http_req_free(TAILQ_FIRST(&http_ep->m_completed_reqs));
    }
    
    if (ws_protocol->m_endpoint_fini) {
        ws_protocol->m_endpoint_fini(http_ep);
    }

    if (http_ep->m_connect_timer) {
        net_timer_free(http_ep->m_connect_timer);
        http_ep->m_connect_timer = NULL;
    }

    http_ep->m_endpoint = NULL;
}

int net_http_endpoint_input(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t ws_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    while(http_ep->m_state == net_http_state_established) {
        net_http_req_t processing_req;
        while((processing_req = TAILQ_FIRST(&http_ep->m_runing_reqs))) {
            int rv = net_http_endpoint_req_input(ws_protocol, processing_req);
            if (rv < 0) return 0;

            switch(processing_req->m_res_state) {
            case net_http_res_state_completed:
                net_http_req_free(processing_req);
                continue; /* */
            default:
                /*当前请求没有完成，比如还需要等待后续数据 */
                return 0;
            }
        }

        if (http_ep->m_upgraded_processor) {
            return http_ep->m_upgraded_processor(http_ep);
        }

        if (!net_endpoint_buf_is_empty(endpoint, net_ep_buf_read)) {
            CPE_ERROR(
                ws_protocol->m_em,
                "http: %s: input without req or upgraded-processor!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(ws_protocol), http_ep->m_endpoint));
            return -1;
        }
    }

    return http_ep->m_state == net_http_state_established ? 0 : -1;
}

int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_disable:
        if (net_http_endpoint_set_state(http_ep, net_http_state_disable) != 0) return -1;
        net_http_endpoint_reset_data(http_ep);
        net_timer_active(http_ep->m_connect_timer, (int32_t)http_ep->m_reconnect_span_ms);
        break;
    case net_endpoint_state_network_error:
        if (net_http_endpoint_set_state(http_ep, net_http_state_error) != 0) return -1;
        net_http_endpoint_reset_data(http_ep);
        net_timer_active(http_ep->m_connect_timer, (int32_t)http_ep->m_reconnect_span_ms);
        break;
    case net_endpoint_state_logic_error:
        if (net_http_endpoint_set_state(http_ep, net_http_state_error) != 0) return -1;
        net_http_endpoint_reset_data(http_ep);
        net_timer_active(http_ep->m_connect_timer, 0);
        break;
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        if (net_http_endpoint_set_state(http_ep, net_http_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (net_http_endpoint_set_state(http_ep, net_http_state_established) != 0) return -1;
        break;
    case net_endpoint_state_deleting:
        assert(0);
        return -1;
    }

    return 0;
}

int net_http_endpoint_write(net_http_endpoint_t http_ep, void const * data, uint32_t size) {
    return net_endpoint_buf_append(http_ep->m_endpoint, net_ep_buf_write, data, size);
}

static void net_http_endpoint_reset_data(net_http_endpoint_t http_ep) {
    while(!TAILQ_EMPTY(&http_ep->m_runing_reqs)) {
        net_http_req_free(TAILQ_FIRST(&http_ep->m_runing_reqs));
    }
    
    while(!TAILQ_EMPTY(&http_ep->m_completed_reqs)) {
        net_http_req_free(TAILQ_FIRST(&http_ep->m_completed_reqs));
    }
}

static void net_http_endpoint_do_connect(net_timer_t timer, void * ctx) {
    net_http_endpoint_t http_ep = ctx;

    if (net_endpoint_connect(http_ep->m_endpoint) != 0) {
        net_timer_active(timer, (int32_t)http_ep->m_reconnect_span_ms);
    }
}

const char * net_http_state_str(net_http_state_t state) {
    switch(state) {
    case net_http_state_disable:
        return "http-disable";
    case net_http_state_connecting:
        return "http-connecting";
    case net_http_state_established:
        return "http-established";
    case net_http_state_error:
        return "http-error";
    }
}