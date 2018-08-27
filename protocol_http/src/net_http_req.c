#include "net_http_req_i.h"

net_http_req_t
net_http_req_create(net_http_endpoint_t http_ep, net_http_req_method_t method, const char * url) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);
    
    net_http_req_t pre_req = TAILQ_FIRST(&http_ep->m_runing_reqs);
    if (pre_req) {
        if (pre_req->m_state != net_http_req_state_runing) {
            CPE_ERROR(http_protocol->m_em, "http: req: create: pre req still in prepare!");
            return NULL;
        }
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

    req->m_id = ++http_ep->m_max_req_id;
    req->m_state = net_http_req_state_prepare_head;
    TAILQ_INSERT_TAIL(&http_ep->m_runing_reqs, req, m_next);
    
    return req;
}

void net_http_req_free(net_http_req_t req) {
    net_http_endpoint_t http_ep = req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    switch(req->m_state) {
    case net_http_req_state_prepare_head:
    case net_http_req_state_prepare_body:
    case net_http_req_state_runing:
        TAILQ_REMOVE(&http_ep->m_runing_reqs, req, m_next);
        break;
    case net_http_req_state_completed:
        TAILQ_REMOVE(&http_ep->m_completed_reqs, req, m_next);
        break;
    }

    req->m_http_ep = (net_http_endpoint_t)http_protocol;
    TAILQ_INSERT_TAIL(&http_protocol->m_free_reqs, req, m_next);
}

void net_http_req_real_free(net_http_req_t req) {
    net_http_protocol_t http_protocol = (net_http_protocol_t)req->m_http_ep;

    TAILQ_REMOVE(&http_protocol->m_free_reqs, req, m_next);

    mem_free(http_protocol->m_alloc, req);
}

uint16_t net_http_req_id(net_http_req_t req) {
    return req->m_id;
}

net_http_req_state_t net_http_req_state(net_http_req_t req) {
    return req->m_state;
}

net_http_endpoint_t net_http_req_ep(net_http_req_t req) {
    return req->m_http_ep;
}


