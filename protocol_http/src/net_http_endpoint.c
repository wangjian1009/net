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
#include "net_http_ssl_ctx_i.h"

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result);
static void net_http_endpoint_do_connect(net_timer_t timer, void * ctx);
static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state);
static int net_http_endpoint_do_ssl_handshake(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep);
static int net_http_endpoint_do_ssl_encode(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep);
static int net_http_endpoint_do_ssl_decode(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep);

net_http_endpoint_t
net_http_endpoint_create(net_driver_t driver, net_endpoint_type_t type, net_http_protocol_t http_protocol) {
    net_endpoint_t endpoint = net_endpoint_create(driver, type, net_protocol_from_data(http_protocol));
    if (endpoint == NULL) return NULL;
    
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

    return http_ep;
}

void net_http_endpoint_free(net_http_endpoint_t http_ep) {
    net_endpoint_free(http_ep->m_endpoint);
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
    return http_ep->m_ssl_ctx ? 1 : 0;
}

net_http_ssl_ctx_t net_http_endpoint_ssl_ctx(net_http_endpoint_t http_ep) {
    return http_ep->m_ssl_ctx;
}

net_http_ssl_ctx_t net_http_endpoint_ssl_enable(net_http_endpoint_t http_ep) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
    if (http_ep->m_state != net_http_state_disable) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: can`t enable ssl in state %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_http_state_str(http_ep->m_state));
        return NULL;
    }

    if (http_ep->m_ssl_ctx == NULL) {
        http_ep->m_ssl_ctx = net_http_ssl_ctx_create(http_ep);
        if (http_ep->m_ssl_ctx == NULL) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: enable ssl fail",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return NULL;
        }
    }
    
    return http_ep->m_ssl_ctx;
}

int net_http_endpoint_ssl_disable(net_http_endpoint_t http_ep) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
    if (http_ep->m_state != net_http_state_disable) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: can`t disable ssl in state %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_http_state_str(http_ep->m_state));
        return -1;
    }

    if (http_ep->m_ssl_ctx) {
        net_http_ssl_ctx_free(http_ep->m_ssl_ctx);
        http_ep->m_ssl_ctx = NULL;
    }
    
    return 0;
}

net_http_connection_type_t net_http_endpoint_connection_type(net_http_endpoint_t http_ep) {
    return http_ep->m_connection_type;
}

int net_http_endpoint_set_remote(net_http_endpoint_t http_ep, const char * url) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);;

    if (http_ep->m_state != net_http_state_disable) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: set remote and path: can`t set remote in state %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_http_state_str(http_ep->m_state));
        return -1;
    }
    
    const char * addr_begin;
    if (cpe_str_start_with(url, "http://")) {
        if (net_http_endpoint_ssl_disable(http_ep) != 0) return -1;
        addr_begin = url + 7;
    }
    else if (cpe_str_start_with(url, "https://")) {
        if (net_http_endpoint_ssl_enable(http_ep) != 0) return -1;
        addr_begin = url + 8;
    }
    else {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: set remote and path: url %s check protocol fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), url);
        return -1;
    }

    const char * addr_end = strchr(addr_begin, '/');

    const char * str_address = NULL;
    const char * path = NULL;
    
    if (addr_end) {
        mem_buffer_t tmp_buffer = net_http_protocol_tmp_buffer(http_protocol);
        mem_buffer_clear_data(tmp_buffer);
        str_address = mem_buffer_strdup_range(tmp_buffer, addr_begin, addr_end);
        if (str_address == NULL) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: set remote and path: url %s address dup fail!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), url);
            return -1;
        }
        path = addr_end;
    }
    else {
        str_address = addr_begin;
    }

    net_address_t address = net_address_create_auto(net_endpoint_schedule(http_ep->m_endpoint), str_address);
    if (address == NULL) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: set remote and path: url %s address format error!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), url);
        return -1;
    }

    if (net_address_port(address) == 0) {
        net_address_set_port(address, http_ep->m_ssl_ctx ? 443 : 80);
    }
        
    if (net_endpoint_set_remote_address(http_ep->m_endpoint, address, 1) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: set remote and path: url %s set remote address fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), url);
        net_address_free(address);
        return -1;
    }

    return 0;
}

void net_http_endpoint_enable(net_http_endpoint_t http_ep) {
    net_timer_active(http_ep->m_connect_timer, 0);
}

int net_http_endpoint_disable(net_http_endpoint_t http_ep) {
    net_timer_cancel(http_ep->m_connect_timer);
    return net_endpoint_set_state(http_ep->m_endpoint,  net_endpoint_state_disable);
}

int net_http_endpoint_set_state(net_http_endpoint_t http_ep, net_http_state_t state) {
    if (http_ep->m_state == state) return 0;
    
    if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 1) {
        net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);;
        CPE_INFO(
            http_protocol->m_em, "http: %s: state %s ==> %s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            net_http_state_str(http_ep->m_state),
            net_http_state_str(state));
    }

    net_http_state_t old_state = http_ep->m_state;
    
    http_ep->m_state = state;

    return net_http_endpoint_notify_state_changed(http_ep, old_state);
}

static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);;
        
    int rv = http_protocol->m_endpoint_on_state_change
        ? http_protocol->m_endpoint_on_state_change(http_ep, old_state)
        : 0;
    
    return rv;
}

int net_http_endpoint_init(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    http_ep->m_endpoint = endpoint;
    http_ep->m_state = net_http_state_disable;
    http_ep->m_connection_type = net_http_connection_type_keep_alive;
    http_ep->m_ssl_ctx = NULL;
    http_ep->m_reconnect_span_ms = 3u * 1000u;
    http_ep->m_max_req_id = 0;
    TAILQ_INIT(&http_ep->m_reqs);
    
    http_ep->m_connect_timer = net_timer_auto_create(schedule, net_http_endpoint_do_connect, http_ep);
    if (http_ep->m_connect_timer == NULL) {
        CPE_ERROR(http_protocol->m_em, "http: ???: init: create connect timer fail!");
        return -1;
    }
    
    if (http_protocol->m_endpoint_init && http_protocol->m_endpoint_init(http_ep) != 0) {
        CPE_ERROR(http_protocol->m_em, "http: ???: init: external init fail!");
        net_timer_free(http_ep->m_connect_timer);
        return -1;
    }
    
    return 0;
}

void net_http_endpoint_fini(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    while(!TAILQ_EMPTY(&http_ep->m_reqs)) {
        net_http_req_t req = TAILQ_FIRST(&http_ep->m_reqs);
        
        if (!req->m_res_ignore && req->m_res_on_complete) {
            req->m_res_on_complete(req->m_res_ctx, req, net_http_res_canceled);
        }

        net_http_req_free(req);
    }
    
    if (http_protocol->m_endpoint_fini) {
        http_protocol->m_endpoint_fini(http_ep);
    }

    if (http_ep->m_ssl_ctx) {
        net_http_ssl_ctx_free(http_ep->m_ssl_ctx);
        http_ep->m_ssl_ctx = NULL;
    }
    
    if (http_ep->m_connect_timer) {
        net_timer_free(http_ep->m_connect_timer);
        http_ep->m_connect_timer = NULL;
    }

    http_ep->m_endpoint = NULL;
}

int net_http_endpoint_input(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    if (http_ep->m_ssl_ctx != NULL) {
        if (http_ep->m_state == net_http_state_connecting
            && net_endpoint_state(http_ep->m_endpoint) == net_endpoint_state_established
            && !net_endpoint_buf_is_empty(endpoint, net_ep_buf_read))
        {
            if (net_http_endpoint_do_ssl_handshake(http_protocol, http_ep) != 0) return -1;
            if (http_ep->m_state != net_http_state_established) return 0;
        }

        if (net_http_endpoint_do_ssl_decode(http_protocol, http_ep) != 0) return -1;
    }
    else {
        if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_http_in, net_ep_buf_read, 0) != 0) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: <<< move read buf to http-in buf fail!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return -1;
        }
    }
    
    while(http_ep->m_state == net_http_state_established && !net_endpoint_buf_is_empty(endpoint, net_ep_buf_http_in)) {
        if (http_ep->m_connection_type != net_http_connection_type_upgrade) {
            net_http_req_t processing_req = TAILQ_FIRST(&http_ep->m_reqs);
            if (processing_req == NULL) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: input without req or upgraded-processor!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
            
            int rv = net_http_endpoint_req_input(http_protocol, http_ep, processing_req);
            if (rv < 0) return -1;

            if (processing_req->m_res_state == net_http_res_state_completed) {
                net_http_req_free(processing_req);

                if (http_ep->m_connection_type == net_http_connection_type_close) {
                    if (net_endpoint_set_state(endpoint, http_ep->m_reconnect_span_ms ? net_endpoint_state_disable : net_endpoint_state_deleting) != 0) {
                        return -1;
                    }
                    else {
                        return 0;
                    }
                }
                else {
                    continue;
                }
            }
            else {
                return 0; /*当前请求没有完成，比如还需要等待后续数据 */
            }
        }
        else {
            if (http_protocol->m_endpoint_upgraded_input == NULL) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: connection upgraded, no input processor!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
            
            if (http_protocol->m_endpoint_upgraded_input(http_ep) != 0) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: upgraded connection process fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
            else {
                return 0;
            }
        }
    }

    return http_ep->m_state == net_http_state_established ? 0 : -1;
}

int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_disable:
        if (net_http_endpoint_set_state(http_ep, net_http_state_disable) != 0) return -1;
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_canceled);
        if (http_ep->m_reconnect_span_ms) {
            net_timer_active(http_ep->m_connect_timer, (int32_t)http_ep->m_reconnect_span_ms);
        }
        break;
    case net_endpoint_state_network_error:
        if (net_http_endpoint_set_state(http_ep, net_http_state_error) != 0) return -1;
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_disconnected);
        if (http_ep->m_reconnect_span_ms) {
            net_timer_active(
                http_ep->m_connect_timer,
                old_state == net_endpoint_state_established
                ? 0
                : (int32_t)http_ep->m_reconnect_span_ms);
        }
        break;
    case net_endpoint_state_logic_error:
        if (net_http_endpoint_set_state(http_ep, net_http_state_error) != 0) return -1;
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_canceled);
        if (http_ep->m_reconnect_span_ms) {
            int64_t ct = cur_time_ms;
            if (http_ep->m_connecting_time_ms
                && ct > http_ep->m_connecting_time_ms
                && ((ct - http_ep->m_connecting_time_ms) < http_ep->m_reconnect_span_ms))
            {
                net_timer_active(http_ep->m_connect_timer, http_ep->m_reconnect_span_ms);
            }
            else {
                net_timer_active(http_ep->m_connect_timer, 0);
            }
        }
        break;
    case net_endpoint_state_resolving:
        if (net_http_endpoint_set_state(http_ep, net_http_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_connecting:
        http_ep->m_connecting_time_ms = cur_time_ms();
        if (net_http_endpoint_set_state(http_ep, net_http_state_connecting) != 0) return -1;
        break;
    case net_endpoint_state_established:
        if (http_ep->m_ssl_ctx) {
            if (net_http_ssl_ctx_setup(http_protocol, http_ep, http_ep->m_ssl_ctx) != 0) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: connection established, setup ssl fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }

            if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em,
                    "http: %s: ssl: handshake begin",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            }
            
            if (net_http_endpoint_do_ssl_handshake(net_http_endpoint_protocol(http_ep), http_ep) != 0) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: connection established, start ssl handshake fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
        }
        else {
            if (net_http_endpoint_set_state(http_ep, net_http_state_established) != 0) return -1;
        }
        break;
    case net_endpoint_state_deleting:
        assert(0);
        return -1;
    }

    return 0;
}

int net_http_endpoint_write(
    net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t http_req, void const * data, uint32_t size)
{
    assert(http_req);
    assert(http_req->m_req_state != net_http_req_state_completed);

    if (net_endpoint_buf_append(http_ep->m_endpoint, net_ep_buf_http_out, data, size) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: req %d: write big data(size=%d) fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            http_req->m_id, size);
        return -1;
    }

    return 0;
}

int net_http_endpoint_flush(net_http_endpoint_t http_ep) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    uint32_t sz = net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_out);
    if (sz > 0) {
        /*dump 提交数据的信息 */
        if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 2) {
            char * buf;
            if (net_endpoint_buf_peak_with_size(http_ep->m_endpoint, net_ep_buf_http_out, sz, (void**)&buf) == 0
                && buf != NULL)
            {
                uint32_t left_sz = sz;
                
                net_http_req_t req;
                TAILQ_FOREACH(req, &http_ep->m_reqs, m_next) {
                    uint32_t req_used_sz = req->m_head_size + req->m_body_size - req->m_flushed_size;
                    if (req_used_sz > left_sz) req_used_sz = left_sz;
                    if (req_used_sz == 0) continue;

                    if (req->m_flushed_size == 0 && req->m_head_size > 4 && req_used_sz >= req->m_head_size) { /*包含完整头部 */
                        char * p = buf + req->m_head_size - 4;
                        *p = 0;
                        
                        CPE_INFO(
                            http_protocol->m_em, "http: %s: req %d: >>> head=%d/%d, body=%d/%d\n%s",
                            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
                            req->m_id,
                            req->m_head_size,
                            req->m_head_size,
                            req_used_sz - req->m_head_size,
                            req->m_body_size,
                            buf);

                        *p = '\r';
                    }
                    else {
                        uint32_t total_flushed_sz = req_used_sz + req->m_flushed_size;
                        uint32_t head_used_sz = req->m_head_size;
                        if (head_used_sz > total_flushed_sz) {
                            head_used_sz = total_flushed_sz;
                        }
                        head_used_sz -= req->m_flushed_size;

                        CPE_INFO(
                            http_protocol->m_em, "http: %s: req %d: >>> head=%d/%d, body=%d, total=%d/%d",
                            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
                            req->m_id,
                            head_used_sz,
                            req->m_head_size,
                            req_used_sz - head_used_sz,
                            req->m_body_size,
                            req->m_head_size + req->m_body_size);
                    }
                }
            }
        }
        
        if (http_ep->m_ssl_ctx) {
            if (net_http_endpoint_do_ssl_encode(http_protocol, http_ep) != 0) return -1;
        }
        else {
            if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_write, net_ep_buf_http_out, 0) != 0) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: <<< move http-out buf to write buf fail!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }
        }
    }

    uint32_t left_sz = sz;
    net_http_req_t req;
    TAILQ_FOREACH(req, &http_ep->m_reqs, m_next) {
        uint32_t req_total_sz = req->m_head_size + req->m_body_size;
        uint32_t req_use_sz = req_total_sz - req->m_flushed_size;
        if (req_use_sz > left_sz) req_use_sz = left_sz;

        req->m_flushed_size += req_use_sz;
        sz -= req_use_sz;

        if (sz == 0) break;
    }
    
    return 0;
}

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result) {
    http_ep->m_connection_type = net_http_connection_type_keep_alive;

    while(!TAILQ_EMPTY(&http_ep->m_reqs)) {
        net_http_req_t req = TAILQ_FIRST(&http_ep->m_reqs);
        
        if (!req->m_res_ignore && req->m_res_on_complete) {
            req->m_res_on_complete(req->m_res_ctx, req, result);
        }

        net_http_req_free(req);
    }
    
    if (http_ep->m_ssl_ctx) {
        net_http_ssl_ctx_reset(http_protocol, http_ep, http_ep->m_ssl_ctx);        
    }
}

static void net_http_endpoint_do_connect(net_timer_t timer, void * ctx) {
    net_http_endpoint_t http_ep = ctx;

    if (net_endpoint_connect(http_ep->m_endpoint) != 0) {
        if (http_ep->m_reconnect_span_ms) {
            net_timer_active(timer, (int32_t)http_ep->m_reconnect_span_ms);
        }
    }
}

static int net_http_endpoint_do_ssl_handshake(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep) {
    int rv = mbedtls_ssl_handshake(&http_ep->m_ssl_ctx->m_ssl);
    if (rv != 0) {
        if(rv == MBEDTLS_ERR_SSL_WANT_READ || rv == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return 0;
        }
        else {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: ssl: handshake fail, rv=%d(%s)!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                rv, net_http_ssl_error_str(rv));
            return -1;
        }
    }
    else {
        if (net_endpoint_protocol_debug(http_ep->m_endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em,
                "http: %s: ssl: handshake success",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
        }

        if (net_http_endpoint_set_state(http_ep, net_http_state_established) != 0) return -1;
    }

    return 0;
}

static int net_http_endpoint_do_ssl_encode(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep) {
    uint32_t data_size = net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_out);
    void * data;
    if (net_endpoint_buf_peak_with_size(http_ep->m_endpoint, net_ep_buf_http_out, data_size, &data) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: >>> ssl encode peak data fail, size=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            data_size);
        return -1;
    }

    int rv = mbedtls_ssl_write(&http_ep->m_ssl_ctx->m_ssl, data, data_size);
    if (rv < 0) {
        if (rv != MBEDTLS_ERR_SSL_WANT_READ && rv != MBEDTLS_ERR_SSL_WANT_WRITE) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: >>> ssl write data fail, size=%d, rv=%d (%s)!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                data_size,
                rv, net_http_ssl_error_str(rv));
            return -1;
        }
    }

    net_endpoint_buf_consume(http_ep->m_endpoint, net_ep_buf_http_out, data_size);
    return 0;
}

static int net_http_endpoint_do_ssl_decode(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep) {
    uint32_t buf_capacity = net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_in);
    void * buf = net_endpoint_buf_alloc(http_ep->m_endpoint, &buf_capacity);
    if (buf == NULL) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: <<< ssl decode alloc buf fail, size=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            buf_capacity);
        return -1;
    }

    int rv = mbedtls_ssl_read(&http_ep->m_ssl_ctx->m_ssl, buf, buf_capacity);
    if (rv < 0) {
        net_endpoint_buf_release(http_ep->m_endpoint);
        
        if(rv == MBEDTLS_ERR_SSL_WANT_READ || rv == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return 0;
        }
        else {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: >>> ssl read data fail, input-sz=%d, buf-capacity=%d, rv=%d (%s)!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_in),
                buf_capacity,
                rv, net_http_ssl_error_str(rv));
            return -1;
        }
    }
    else {
        if (net_endpoint_buf_supply(http_ep->m_endpoint, net_ep_buf_http_in, (uint32_t)rv) != 0) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: >>> ssl read data success, but supply data fail",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return -1;
        }

        return 0;
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
