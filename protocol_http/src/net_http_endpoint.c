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
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_http_endpoint_i.h"
#include "net_http_protocol_i.h"
#include "net_http_req_i.h"

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result);

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

net_schedule_t net_http_endpoint_schedule(net_http_endpoint_t http_ep) {
    return net_endpoint_schedule(http_ep->m_endpoint);
}

net_endpoint_t net_http_endpoint_base_endpoint(net_http_endpoint_t http_ep) {
    return http_ep->m_endpoint;
}

net_http_protocol_t net_http_endpoint_protocol(net_http_endpoint_t http_ep) {
    return net_protocol_data(net_endpoint_protocol(http_ep->m_endpoint));
}

net_http_connection_type_t net_http_endpoint_connection_type(net_http_endpoint_t http_ep) {
    return http_ep->m_connection_type;
}

int net_http_endpoint_init(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_schedule_t schedule = net_endpoint_schedule(endpoint);

    http_ep->m_endpoint = endpoint;
    http_ep->m_connection_type = net_http_connection_type_keep_alive;
    http_ep->m_req_count = 0;
    TAILQ_INIT(&http_ep->m_reqs);
    
    return 0;
}

void net_http_endpoint_fini(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_canceled);
    http_ep->m_endpoint = NULL;
}

int net_http_endpoint_input(net_endpoint_t endpoint) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    /* uint32_t isz = net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_read); */
    /* void * idata = NULL; */
    /* net_endpoint_buf_peak_with_size(http_ep->m_endpoint, net_ep_buf_read, isz, &idata); */

    /* CPE_ERROR( */
    /*     http_protocol->m_em, */
    /*     "http: <<< input buf %d\n%s", */
    /*     isz, */
    /*     mem_buffer_dump_data(net_http_protocol_tmp_buffer(http_protocol), idata, isz, 0)); */

    if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_http_in, net_ep_buf_read, 0) != 0) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: <<< move read buf to http-in buf fail!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
        return -1;
    }

    while(!net_endpoint_buf_is_empty(endpoint, net_ep_buf_http_in)) {
        uint32_t before_process_size = net_endpoint_buf_size(endpoint, net_ep_buf_http_in);

        if (http_ep->m_connection_type != net_http_connection_type_upgrade) {
            if (net_http_endpoint_do_process(http_protocol, http_ep, endpoint) != 0) return -1;

            if (http_ep->m_current_res.m_state == net_http_res_state_completed) {
                assert(http_ep->m_current_res.m_req);
                net_http_req_t to_free = http_ep->m_current_res.m_req;
                http_ep->m_current_res.m_req = NULL;
                net_http_req_free_force(to_free);

                bzero(&http_ep->m_current_res, sizeof(http_ep->m_current_res));
                http_ep->m_current_res.m_state = net_http_res_state_reading_head_first;
                    
                if (http_ep->m_connection_type == net_http_connection_type_close) {
                    if (net_endpoint_set_state(endpoint, net_endpoint_state_disable) != 0) {
                        net_endpoint_set_state(endpoint, net_endpoint_state_deleting);
                        return -1;
                    }
                }
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

        if (net_endpoint_state(endpoint) == net_endpoint_state_deleting) break;
        
        if (before_process_size == net_endpoint_buf_size(endpoint, net_ep_buf_http_in)) {
            CPE_ERROR(
                http_protocol->m_em,
                "http: %s: process http input, buf-size=%d, no any processed",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                before_process_size);
            break;
        }
    }

    return 0;
}

int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t old_state) {
    net_http_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_read_closed:
    case net_endpoint_state_write_closed:
        break;
    case net_endpoint_state_disable:
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_conn_disconnected);
        break;
    case net_endpoint_state_error:
        net_http_endpoint_reset_data(http_protocol, http_ep, net_http_res_conn_error);
        break;
    case net_endpoint_state_resolving:
        break;
    case net_endpoint_state_connecting:
        http_ep->m_connecting_time_ms = net_schedule_cur_time_ms(net_endpoint_schedule(endpoint));
        break;
    case net_endpoint_state_established:
        break;
    case net_endpoint_state_deleting:
        assert(0);
        return -1;
    }

    return 0;
}

int net_http_endpoint_write_head_pair(
    net_http_endpoint_t http_ep, net_http_req_t http_req, const char * attr_name, const char * attr_value)
{
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
    uint32_t name_len = strlen(attr_name);
    uint32_t value_len = strlen(attr_value);
    uint32_t total_sz = name_len + value_len + 4;

    uint32_t buf_sz = total_sz;
    uint8_t * buf = net_endpoint_buf_alloc_at_least(http_ep->m_endpoint, &buf_sz);
    if (buf == NULL) {
        CPE_ERROR(
            http_protocol->m_em,
            "http: %s: write head pair, alloc buf fail, size=%d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            total_sz);
        return -1;
    }

    memcpy(buf, attr_name, name_len); buf += name_len;
    memcpy(buf, ": ", 2); buf += 2;
    memcpy(buf, attr_value, value_len); buf += value_len;
    memcpy(buf, "\r\n", 2);

    if (net_endpoint_buf_supply(http_ep->m_endpoint, net_ep_buf_http_out, total_sz) != 0) return -1;

    http_req->m_head_size += total_sz;
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
            if (req_total_sz == 0) { /*没有任何数据需要处理 */
                if (req->m_is_free) { /*已经释放，则继续处理后续请求 */
                    net_http_req_free_force(req);
                    continue;
                }
                else { /*没有释放，则等待数据到来 */
                    return 0;
                }
            }

            if (req->m_flushed_size == 0) { /*没有发送任何数据 */
                if (req->m_is_free) { /*已经释放，还没有发送，则把需要发送的数据清理掉 */
                    uint32_t supply_size = req->m_head_size + req->m_body_supply_size;
                    assert(net_endpoint_buf_size(http_ep->m_endpoint, net_ep_buf_http_out) >= supply_size);
                    net_endpoint_buf_consume(http_ep->m_endpoint, net_ep_buf_http_out, supply_size);
                    assert(supply_size <= buf_sz);
                    buf_sz -= supply_size;
                    net_http_req_free_force(req);
                    continue;
                }
            }

            assert(req_total_sz >= req->m_flushed_size);
            uint32_t req_to_send_sz = req_total_sz - req->m_flushed_size;
            if (req_to_send_sz == 0) continue;

            if (req_to_send_sz > buf_sz) req_to_send_sz = buf_sz;

            if (net_endpoint_buf_append_from_self(http_ep->m_endpoint, net_ep_buf_write, net_ep_buf_http_out, req_to_send_sz) != 0) {
                CPE_ERROR(
                    http_protocol->m_em,
                    "http: %s: <<< move http-out buf to write buf fail!",
                    net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
                return -1;
            }

            req->m_flushed_size += req_to_send_sz;
            buf_sz -= req_to_send_sz;
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

static void net_http_endpoint_reset_data(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_res_result_t result) {
    http_ep->m_connection_type = net_http_connection_type_keep_alive;

    bzero(&http_ep->m_current_res, sizeof(http_ep->m_current_res));
    
    while(!TAILQ_EMPTY(&http_ep->m_reqs)) {
        net_http_req_t req = TAILQ_FIRST(&http_ep->m_reqs);

        if (!req->m_on_complete_processed
            && req->m_res_on_complete)
        {
            req->m_on_complete_processed = 1;
            req->m_res_on_complete(req->m_res_ctx, req, result, NULL, 0);
        }

        net_http_req_free_force(req);
    }
}
