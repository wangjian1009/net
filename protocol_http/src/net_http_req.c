#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_http_req_i.h"

static void net_http_req_on_timeout(net_timer_t timer, void * ctx);

net_http_req_t
net_http_req_create(net_http_endpoint_t http_ep, net_http_req_method_t method, const char * url) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
    net_http_req_t pre_req = TAILQ_FIRST(&http_ep->m_reqs);
    if (pre_req) {
        if (pre_req->m_req_state != net_http_req_state_completed) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req: create: pre req still in prepare!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
            return NULL;
        }
    }

    if (http_ep->m_connection_type == net_http_connection_type_upgrade) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req: create: connection is already upgraded!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint));
        return NULL;
    }
    
    net_http_req_t req = TAILQ_FIRST(&http_protocol->m_free_reqs);
    if (req) {
        TAILQ_REMOVE(&http_protocol->m_free_reqs, req, m_next);
    }
    else {
        req = mem_alloc(http_protocol->m_alloc, sizeof(struct net_http_req));
        if (req == NULL) {
            CPE_ERROR(http_protocol->m_em, "http: req: create: alloc fail!");
            return NULL;
        }
    }

    req->m_http_ep = http_ep;
    req->m_id = ++http_protocol->m_max_req_id;
    req->m_data_sended = 0;
    req->m_is_free = 0;
    req->m_on_complete_processed = 0;
    req->m_on_complete_processed = 0;

    req->m_req_method = method;
    req->m_req_state = net_http_req_state_prepare_head;
    req->m_req_have_keep_alive = 0;
    req->m_head_size = 0;
    req->m_body_size = 0;
    req->m_flushed_size = 0;

    req->m_res_ignore = 0;
    req->m_res_ctx = NULL;
    req->m_res_on_begin = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_body = NULL;
    req->m_res_on_complete = NULL;
    req->m_res_code = 0;
    req->m_res_message[0] = 0;

    http_ep->m_req_count++;
    TAILQ_INSERT_TAIL(&http_ep->m_reqs, req, m_next);
    
    if (net_http_req_do_send_first_line(http_protocol, req, method, url) != 0) {
        CPE_ERROR(http_protocol->m_em, "http: req: create: write req first line fail!");
        net_http_req_free_force(req);
        return NULL;
    }
    
    return req;
}

void net_http_req_free_force(net_http_req_t req) {
    net_http_endpoint_t http_ep = req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    assert(http_ep->m_current_res.m_req != req);

    net_http_req_clear_reader(req);
    
    assert(http_ep->m_req_count > 0);
    http_ep->m_req_count--;
    TAILQ_REMOVE(&http_ep->m_reqs, req, m_next);
    
    req->m_http_ep = (net_http_endpoint_t)http_protocol;
    TAILQ_INSERT_TAIL(&http_protocol->m_free_reqs, req, m_next);
}

void net_http_req_free(net_http_req_t req) {
    /*数据还没有发送，或者响应没有收到，则标记后返回，等待后续清理 */
    if (req->m_flushed_size < (req->m_head_size + req->m_body_size)
        || !req->m_res_completed)
    {
        net_http_req_clear_reader(req);
        req->m_is_free = 1;
        return;
    }

    net_http_req_free_force(req);
}

void net_http_req_real_free(net_http_req_t req) {
    net_http_protocol_t http_protocol = (net_http_protocol_t)req->m_http_ep;

    TAILQ_REMOVE(&http_protocol->m_free_reqs, req, m_next);

    mem_free(http_protocol->m_alloc, req);
}

net_http_req_t net_http_req_find(net_http_endpoint_t http_ep, uint16_t req_id) {
    net_http_req_t req;

    TAILQ_FOREACH(req, &http_ep->m_reqs, m_next) {
        if (req->m_id == req_id) {
            if (req->m_is_free) {
                return NULL;
            }
            else {
                return req;
            }
        }
    }

    return NULL;
}

uint16_t net_http_req_id(net_http_req_t req) {
    return req->m_id;
}

net_http_endpoint_t net_http_req_ep(net_http_req_t req) {
    return req->m_http_ep;
}

int net_http_req_set_reader(
    net_http_req_t req,
    void * ctx,
    net_http_req_on_res_begin_fun_t on_begin,
    net_http_req_on_res_head_fun_t on_head,
    net_http_req_on_res_body_fun_t on_body,
    net_http_req_on_res_complete_fun_t on_complete)
{
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(req->m_http_ep);
    
    if (req->m_http_ep->m_current_res.m_req == req) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req is in process, can`t set reader!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
            req->m_id);
        return -1;
    }

    req->m_res_ctx = ctx;
    req->m_res_on_begin = on_begin;
    req->m_res_on_head = on_head;
    req->m_res_on_body = on_body;
    req->m_res_on_complete = on_complete;
    
    return 0;
}

void net_http_req_clear_reader(net_http_req_t req) {
    req->m_res_ctx = NULL;
    req->m_res_on_begin = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_body = NULL;
    req->m_res_on_complete = NULL;
}

uint16_t net_http_req_res_code(net_http_req_t req) {
    return req->m_res_code;
}

const char * net_http_req_res_message(net_http_req_t req) {
    return req->m_res_message;
}

uint32_t net_http_req_res_length(net_http_req_t req) {
    if (req->m_http_ep->m_current_res.m_req != req) {
        return 0;
    }

    return req->m_http_ep->m_current_res.m_trans_encoding == net_http_trans_encoding_none
        ? req->m_http_ep->m_current_res.m_res_content.m_length
        : 0;
}

void net_http_req_cancel_and_free(net_http_req_t req) {
    if (!req->m_res_ignore && !req->m_on_complete_processed && req->m_res_on_complete) {
        req->m_on_complete_processed = 1;
        req->m_res_on_complete(req->m_res_ctx, req, net_http_res_canceled);
    }
    net_http_req_free(req);
}

void net_http_req_cancel_and_free_by_id(net_http_endpoint_t http_ep, uint16_t req_id) {
    net_http_req_t req = net_http_req_find(http_ep, req_id);
    if (req) {
        net_http_req_cancel_and_free(req);
    }
}

const char * net_http_res_state_str(net_http_res_state_t res_state) {
    switch(res_state) {
    case net_http_res_state_reading_head:
        return "http-res-reading-head";
    case net_http_res_state_reading_body:
        return "http-res-reading-body";
    case net_http_res_state_completed:
        return "http-res-completed";
    }
}

const char * net_http_res_result_str(net_http_res_result_t res_result) {
    switch(res_result) {
    case net_http_res_complete:
        return "http-res-complete";
    case net_http_res_timeout:
        return "http-res-timeout";
    case net_http_res_canceled:
        return "http-res-canceled";
    case net_http_res_conn_error:
        return "http-res-conn-error";
    case net_http_res_conn_disconnected:
        return "http-res-conn-disconnected";
    }
}

static void net_http_req_on_timeout(net_timer_t timer, void * ctx) {
    net_http_req_t req = ctx;

    if (!req->m_res_ignore
        && !req->m_on_complete_processed
        && req->m_res_on_complete)
    {
        req->m_on_complete_processed = 1;
        req->m_res_on_complete(req->m_res_ctx, req, net_http_res_timeout);
    }

    net_http_req_free(req);
}
