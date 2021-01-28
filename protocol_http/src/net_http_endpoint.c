#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/time_utils.h"
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

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result);
static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state);

net_http_endpoint_t
net_http_endpoint_create(net_driver_t driver, net_http_protocol_t http_protocol) {
    net_endpoint_t endpoint = net_endpoint_create(driver, net_protocol_from_data(http_protocol), NULL);
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

net_http_protocol_t net_http_endpoint_protocol(net_http_endpoint_t http_ep) {
    return net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));
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
        addr_begin = url + 7;
    }
    else if (cpe_str_start_with(url, "https://")) {
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

    /* if (net_address_port(address) == 0) { */
    /*     net_address_set_port(address, http_ep->m_ssl_ctx ? 443 : 80); */
    /* } */
        
    if (net_endpoint_set_remote_address(http_ep->m_endpoint, address) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: set remote and path: url %s set remote address fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), url);
        net_address_free(address);
        return -1;
    }
    net_address_free(address);

    return 0;
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

    switch (http_ep->m_state) {
    case net_http_state_disable:
    case net_http_state_error:
        http_ep->m_connection_type = net_http_connection_type_keep_alive;
        break;
    default:
        break;
    }
    
    if (net_http_endpoint_notify_state_changed(http_ep, old_state) != 0) return -1;

    return 0;
}

static int net_http_endpoint_notify_state_changed(net_http_endpoint_t http_ep, net_http_state_t old_state) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);;
    return 0;
}

int net_http_endpoint_init(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    http_ep->m_endpoint = endpoint;
    http_ep->m_state = net_http_state_disable;
    http_ep->m_connection_type = net_http_connection_type_keep_alive;
    http_ep->m_request_id_tag = NULL;
    http_ep->m_req_count = 0;
    TAILQ_INIT(&http_ep->m_reqs);
    
    return 0;
}

void net_http_endpoint_fini(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    while(!TAILQ_EMPTY(&http_ep->m_reqs)) {
        net_http_req_cancel_and_free_i(TAILQ_FIRST(&http_ep->m_reqs), 1);
    }

    if (http_ep->m_request_id_tag) {
        mem_free(http_protocol->m_alloc, http_ep->m_request_id_tag);
        http_ep->m_request_id_tag = NULL;
    }

    http_ep->m_endpoint = NULL;
}

int net_http_endpoint_input(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_http_in, net_ep_buf_read, 0) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: <<< move read buf to http-in buf fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
        return -1;
    }

    while(http_ep->m_state == net_http_state_established && !net_endpoint_buf_is_empty(endpoint, net_ep_buf_http_in)) {
        if (http_ep->m_connection_type != net_http_connection_type_upgrade) {
            if (net_http_endpoint_do_process(http_protocol, http_ep, endpoint) != 0) return -1;
            
            if (http_ep->m_current_res.m_state == net_http_res_state_completed) {
                if (http_ep->m_current_res.m_req) {
                    net_http_req_free_i(http_ep->m_current_res.m_req, 1);
                    assert(http_ep->m_current_res.m_req == NULL);
                }

                bzero(&http_ep->m_current_res, sizeof(http_ep->m_current_res));
                http_ep->m_current_res.m_state = net_http_res_state_reading_head;
                    
                if (http_ep->m_connection_type == net_http_connection_type_close) {
                    if (net_endpoint_set_state(endpoint, net_endpoint_state_disable) != 0) {
                        return -1;
                    }
                    else {
                        return 0;
                    }
                }
                else {
                    //if (net_http_endpoint_flush(http_ep) != 0) return -1;
                    continue;
                }
            }
            else {
                return 0; /*当前请求没有完成，比如还需要等待后续数据 */
            }
        }
        else {
            /* if (http_protocol->m_endpoint_upgraded_input == NULL) { */
            /*     CPE_ERROR( */
            /*         http_protocol->m_em, */
            /*         "http: %s: connection upgraded, no input processor!", */
            /*         net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint)); */
            /*     return -1; */
            /* } */
            
            /* if (http_protocol->m_endpoint_upgraded_input(http_ep) != 0) { */
            /*     CPE_ERROR( */
            /*         http_protocol->m_em, */
            /*         "http: %s: upgraded connection process fail", */
            /*         net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint)); */
            /*     return -1; */
            /* } */
            /* else { */
            /*     return 0; */
            /* } */
        }
    }

    return http_ep->m_state == net_http_state_established ? 0 : -1;
}

int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
        break;
    case net_endpoint_state_disable:
        net_http_endpoint_set_state(http_ep, net_http_state_disable);
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_canceled);
        break;
    case net_endpoint_state_error:
        net_http_endpoint_set_state(http_ep, net_http_state_error);
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_canceled);
        break;
    case net_endpoint_state_resolving:
        if (net_http_endpoint_set_state(http_ep, net_http_state_connecting) != 0) {
            net_http_endpoint_set_state(http_ep, net_http_state_error);
        }
        break;
    case net_endpoint_state_connecting:
        http_ep->m_connecting_time_ms = cur_time_ms();
        if (net_http_endpoint_set_state(http_ep, net_http_state_connecting) != 0) {
            net_http_endpoint_set_state(http_ep, net_http_state_error);
        }
        break;
    case net_endpoint_state_established:
        net_http_endpoint_set_state(http_ep, net_http_state_established);
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

    if (http_ep->m_connection_type == net_http_connection_type_upgrade) {
        if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_write, net_ep_buf_http_out, 0) != 0) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: <<< move http-out buf to write buf fail!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return -1;
        }
    }
    else {
        uint32_t buf_sz = net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_out);
        if (buf_sz == 0) return 0;

        net_http_req_t req, next_req;
        for(req = TAILQ_FIRST(&http_ep->m_reqs); buf_sz > 0 && req != NULL; req = next_req) {
            next_req = TAILQ_NEXT(req, m_next);

            uint32_t req_total_sz = req->m_head_size + req->m_body_size;
            uint32_t req_sz = req_total_sz - req->m_flushed_size;
            if (req_sz == 0) {
                if (req->m_free_after_processed) {
                    if (!req->m_data_sended) {
                        CPE_INFO(
                            http_protocol->m_em, "http: %s: req %d skip(no data send)",
                            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                            req->m_id);
                        net_http_req_free_i(req, 1);
                    }
                    else if (http_ep->m_request_id_tag != NULL) {
                        CPE_INFO(
                            http_protocol->m_em, "http: %s: req %d skip(send complete, ep support id match)",
                            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                            req->m_id);
                        net_http_req_free_i(req, 1);
                    }
                }
                continue;
            }

            char * buf;
            if (net_endpoint_buf_peak_with_size(http_ep->m_endpoint, net_ep_buf_http_out, buf_sz, (void**)&buf) != 0 || buf == NULL) {
                CPE_ERROR(
                    http_protocol->m_em, "http: %s: peek size %d fail",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint), buf_sz);
                return -1;
            }

            if (req_sz > buf_sz) req_sz = buf_sz;
        
            if (req->m_flushed_size == 0) {
                if (req_sz < req->m_head_size) { /*头部确保单次发送 */
                    return 0;
                }

                if (req->m_head_size > 4 && req_sz >= req->m_head_size) {
                    char * p = buf + req->m_head_size - 4;
                    *p = 0;
                        
                    CPE_INFO(
                        http_protocol->m_em, "http: %s: req %d: >>> head=%d/%d, body=%d/%d\n%s",
                        net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
                        req->m_id,
                        req->m_head_size,
                        req->m_head_size,
                        req_sz - req->m_head_size,
                        req->m_body_size,
                        buf);

                    *p = '\r';
                }
            }

            if (req->m_data_sended || !req->m_free_after_processed) {
                if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_write, net_ep_buf_http_out, req_sz) != 0) {
                    CPE_ERROR(
                        http_protocol->m_em,
                        "http: %s: <<< move http-out buf to write buf fail!",
                        net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                    return -1;
                }
                req->m_data_sended = 1;
            }
            
            req->m_flushed_size += req_sz;
            buf_sz -= req_sz;
        }
        
        if (buf_sz > 0) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: have data, but no req",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return -1;
        }
    }

    return 0;
}

const char * net_http_endpoint_request_id_tag(net_http_endpoint_t http_endpoint) {
    return http_endpoint->m_request_id_tag ? http_endpoint->m_request_id_tag : "";
}

int net_http_endpoint_set_request_id_tag(net_http_endpoint_t http_endpoint, const char * tag) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_endpoint);
    char * new_tag = NULL;

    if (tag) {
        new_tag = cpe_str_mem_dup(http_protocol->m_alloc, tag);
        if (new_tag == NULL) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: dup tag %s fal",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_endpoint->m_endpoint),
                tag);
            return -1;
        }
    }

    if (http_endpoint->m_request_id_tag) {
        mem_free(http_protocol->m_alloc, http_endpoint->m_request_id_tag);
    }

    http_endpoint->m_request_id_tag = new_tag;
    
    return 0;
}

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result) {
    http_ep->m_connection_type = net_http_connection_type_keep_alive;

    while(!TAILQ_EMPTY(&http_ep->m_reqs)) {
        net_http_req_t req = TAILQ_FIRST(&http_ep->m_reqs);

        if (!req->m_res_ignore
            && !req->m_on_complete_processed
            && req->m_res_on_complete)
        {
            req->m_on_complete_processed = 1;
            req->m_res_on_complete(req->m_res_ctx, req, result);
        }

        net_http_req_free_i(req, 1);
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
