#include "cpe/utils/stream_buffer.h"
#include "net_endpoint.h"
#include "net_http_req_i.h"

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
    req->m_id = ++http_ep->m_max_req_id;
    
    req->m_req_state = net_http_req_state_prepare_head;
    req->m_req_have_keep_alive = 0;
    req->m_head_size = 0;
    req->m_body_size = 0;
    req->m_flushed_size = 0;

    req->m_res_state = net_http_res_state_reading_head;
    req->m_res_ignore = 0;
    req->m_res_trans_encoding = net_http_trans_encoding_none;
    req->m_res_content.m_length = 0;
    req->m_res_ctx = NULL;
    req->m_res_on_begin = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_body = NULL;
    req->m_res_on_complete = NULL;
    req->m_res_code = 0;

    TAILQ_INSERT_TAIL(&http_ep->m_reqs, req, m_next);
    
    if (net_http_req_do_send_first_line(http_protocol, req, method, url) != 0) {
        CPE_ERROR(http_protocol->m_em, "http: req: create: write req first line fail!");
        net_http_req_free(req);
        return NULL;
    }
    
    return req;
}

void net_http_req_free(net_http_req_t req) {
    net_http_endpoint_t http_ep = req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    TAILQ_REMOVE(&http_ep->m_reqs, req, m_next);

    req->m_http_ep = (net_http_endpoint_t)http_protocol;
    TAILQ_INSERT_TAIL(&http_protocol->m_free_reqs, req, m_next);
}

void net_http_req_real_free(net_http_req_t req) {
    net_http_protocol_t http_protocol = (net_http_protocol_t)req->m_http_ep;

    TAILQ_REMOVE(&http_protocol->m_free_reqs, req, m_next);

    mem_free(http_protocol->m_alloc, req);
}

const char * net_http_res_result_str(net_http_res_result_t res_result) {
    switch(res_result) {
    case net_http_res_complete:
        return "http-res-complete";
    case net_http_res_timeout:
        return "http-res-timeout";
    case net_http_res_canceled:
        return "http-res-canceled";
    case net_http_res_disconnected:
        return "http-res-disconnected";
    }
}

uint16_t net_http_req_id(net_http_req_t req) {
    return req->m_id;
}

net_http_endpoint_t net_http_req_ep(net_http_req_t req) {
    return req->m_http_ep;
}
