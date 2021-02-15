#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_http2_req_i.h"
#include "net_http2_stream_i.h"

static void net_http2_req_on_timeout(net_timer_t timer, void * ctx);

net_http2_req_t
net_http2_req_create(net_http2_endpoint_t http_ep) {
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));
    
    net_http2_req_t req = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_req));
    if (req == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req: create: alloc fail!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint));
        return NULL;
    }

    req->m_endpoint = http_ep;
    req->m_stream = NULL;
    req->m_id = ++protocol->m_max_req_id;
    req->m_free_after_processed = 0;
    req->m_on_complete_processed = 0;
    req->m_on_complete_processed = 0;

    req->m_req_state = net_http2_req_state_init;
    req->m_req_head_count = 0;
    req->m_req_head_capacity = 0;
    req->m_req_headers = NULL;
    
    req->m_res_ctx = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_data = NULL;
    req->m_res_on_complete = NULL;
    req->m_res_head_count = 0;
    req->m_res_head_capacity = 0;
    req->m_res_headers = NULL;

    http_ep->m_req_count++;
    TAILQ_INSERT_TAIL(&http_ep->m_reqs, req, m_next_for_endpoint);

    return req;
}

void net_http2_req_free_i(net_http2_req_t req, uint8_t force) {
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    req->m_res_ctx = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_data = NULL;
    req->m_res_on_complete = NULL;

    /*req*/
    uint16_t i;
    for(i = 0; i < req->m_req_head_count; ++i) {
        mem_free(protocol->m_alloc, req->m_req_headers[i].name);
        mem_free(protocol->m_alloc, req->m_req_headers[i].value);
    }
    req->m_req_head_count = 0;
    
    if (req->m_req_headers) {
        mem_free(protocol->m_alloc, req->m_req_headers);
        req->m_req_headers = NULL;
        req->m_req_head_capacity = 0;
    }
            
    /*res*/
    for(i = 0; i < req->m_res_head_count; ++i) {
        mem_free(protocol->m_alloc, req->m_res_headers[i].m_name);
        mem_free(protocol->m_alloc, req->m_res_headers[i].m_value);
    }
    req->m_res_head_count = 0;
    
    if (req->m_res_headers) {
        mem_free(protocol->m_alloc, req->m_res_headers);
        req->m_res_headers = NULL;
        req->m_res_head_capacity = 0;
    }

    /*stream*/
    if (req->m_stream) {
        net_http2_req_set_stream(req, NULL);
        assert(req->m_stream == NULL);
    }

    assert(http_ep->m_req_count > 0);
    http_ep->m_req_count--;
    TAILQ_REMOVE(&http_ep->m_reqs, req, m_next_for_endpoint);
    
    mem_free(protocol->m_alloc, req);
}

void net_http2_req_free(net_http2_req_t req) {
    net_http2_req_free_i(req, 0);
}

net_http2_req_t
net_http2_req_find(net_http2_endpoint_t http_ep, uint16_t req_id) {
    net_http2_req_t req;

    TAILQ_FOREACH(req, &http_ep->m_reqs, m_next_for_endpoint) {
        if (req->m_id == req_id) {
            if (req->m_free_after_processed) {
                return NULL;
            }
            else {
                return req;
            }
        }
    }

    return NULL;
}

uint16_t net_http2_req_id(net_http2_req_t req) {
    return req->m_id;
}

net_http2_endpoint_t net_http2_req_endpoint(net_http2_req_t req) {
    return req->m_endpoint;
}

net_http2_stream_t net_http2_req_stream(net_http2_req_t req) {
    return req->m_stream;
}

int net_http2_req_set_reader(
    net_http2_req_t req,
    void * ctx,
    net_http2_req_on_res_head_fun_t on_head,
    net_http2_req_on_res_data_fun_t on_data,
    net_http2_req_on_res_complete_fun_t on_complete)
{
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));
    
    req->m_res_ctx = ctx;
    req->m_res_on_head = on_head;
    req->m_res_on_data = on_data;
    req->m_res_on_complete = on_complete;
    
    return 0;
}

void net_http2_req_clear_reader(net_http2_req_t req) {
    req->m_res_ctx = NULL;
    req->m_res_on_head = NULL;
    req->m_res_on_data = NULL;
    req->m_res_on_complete = NULL;
}

void net_http2_req_cancel_and_free_i(net_http2_req_t req, uint8_t force) {
    if (!req->m_on_complete_processed && req->m_res_on_complete) {
        req->m_on_complete_processed = 1;
        req->m_res_on_complete(req->m_res_ctx, req, net_http2_res_canceled);
    }
    net_http2_req_free_i(req, force);
}

void net_http2_req_cancel_and_free(net_http2_req_t req) {
    net_http2_req_cancel_and_free_i(req, 0);
}

void net_http2_req_cancel_and_free_by_id(net_http2_endpoint_t http_ep, uint16_t req_id) {
    net_http2_req_t req = net_http2_req_find(http_ep, req_id);
    if (req) {
        net_http2_req_cancel_and_free(req);
    }
}

const char * net_http2_res_state_str(net_http2_res_state_t res_state) {
    switch(res_state) {
    case net_http2_res_state_reading_head:
        return "http-res-reading-head";
    case net_http2_res_state_reading_body:
        return "http-res-reading-body";
    case net_http2_res_state_completed:
        return "http-res-completed";
    }
}

const char * net_http2_res_result_str(net_http2_res_result_t res_result) {
    switch(res_result) {
    case net_http2_res_complete:
        return "http-res-complete";
    case net_http2_res_timeout:
        return "http-res-timeout";
    case net_http2_res_canceled:
        return "http-res-canceled";
    case net_http2_res_disconnected:
        return "http-res-disconnected";
    }
}

static void net_http2_req_on_timeout(net_timer_t timer, void * ctx) {
    net_http2_req_t req = ctx;

    if (!req->m_on_complete_processed
        && req->m_res_on_complete)
    {
        req->m_on_complete_processed = 1;
        req->m_res_on_complete(req->m_res_ctx, req, net_http2_res_timeout);
    }

    net_http2_req_free(req);
}

void net_http2_req_set_stream(net_http2_req_t req, net_http2_stream_t stream) {
    if (req->m_stream == stream) return;

    if (req->m_stream) {
        assert(req->m_stream->m_cli.m_req == req);
        req->m_stream->m_cli.m_req = NULL;
        req->m_stream = NULL;
    }

    req->m_stream = stream;

    if (req->m_stream) {
        assert(req->m_stream->m_runing_mode == net_http2_stream_runing_mode_cli);
        assert(req->m_stream->m_cli.m_req == NULL);
        req->m_stream->m_cli.m_req = req;
    }
}

const char *  net_http2_req_method_str(net_http2_req_method_t method) {
    switch(method) {
    case net_http2_req_method_get:
        return "GET";
    case net_http2_req_method_post:
        return "POST";
    case net_http2_req_method_head:
        return "HEAD";
    }
}
