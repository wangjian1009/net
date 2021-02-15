#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_timer.h"
#include "net_http2_req_i.h"
#include "net_http2_stream_i.h"

net_http2_req_t
net_http2_req_create(net_http2_endpoint_t http_ep) {
    net_http2_protocol_t protocol =
        net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));
    
    net_http2_req_t req = mem_alloc(protocol->m_alloc, sizeof(struct net_http2_req));
    if (req == NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req: create: alloc fail!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(http_ep->m_runing_mode));
        return NULL;
    }

    req->m_endpoint = http_ep;
    req->m_stream = NULL;
    req->m_id = ++protocol->m_max_req_id;
    req->m_state = net_http2_req_state_init;

    req->m_req_head_count = 0;
    req->m_req_head_capacity = 0;
    req->m_req_headers = NULL;
    
    req->m_res_head_count = 0;
    req->m_res_head_capacity = 0;
    req->m_res_headers = NULL;

    req->m_read_ctx = NULL;
    req->m_read_ctx_free = NULL;
    req->m_on_state_change = NULL;
    req->m_on_recv = NULL;

    req->m_write_ctx = NULL;
    req->m_write_ctx_free = NULL;
    req->m_on_write = NULL;
    
    http_ep->m_req_count++;
    TAILQ_INSERT_TAIL(&http_ep->m_reqs, req, m_next_for_endpoint);

    return req;
}

void net_http2_req_free(net_http2_req_t req) {
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (req->m_write_ctx_free) {
        req->m_write_ctx_free(req->m_write_ctx);
        req->m_write_ctx_free = NULL;
    }

    if (req->m_read_ctx_free) {
        req->m_read_ctx_free(req->m_read_ctx);
        req->m_read_ctx_free = NULL;
    }
    
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

net_http2_req_t
net_http2_req_find(net_http2_endpoint_t http_ep, uint16_t req_id) {
    net_http2_req_t req;

    TAILQ_FOREACH(req, &http_ep->m_reqs, m_next_for_endpoint) {
        if (req->m_id == req_id) {
            return req;
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
    void * read_ctx,
    net_http2_req_on_state_change_fun_t on_state_change,
    net_http2_req_on_recv_fun_t on_recv,
    void (*read_ctx_free)(void *))
{
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (req->m_read_ctx_free) {
        req->m_read_ctx_free(req->m_read_ctx);
    }
    
    req->m_read_ctx = read_ctx;
    req->m_on_state_change = on_state_change;
    req->m_on_recv = on_recv;
    req->m_read_ctx_free = read_ctx_free;

    return 0;
}

void net_http2_req_clear_reader(net_http2_req_t req) {
    req->m_read_ctx = NULL;
    req->m_on_state_change = NULL;
    req->m_on_recv = NULL;
    req->m_read_ctx_free = NULL;
}

int net_http2_req_set_writer(
    net_http2_req_t req,
    void * write_ctx,
    net_http2_req_on_write_fun_t on_write,
    void (*write_ctx_free)(void *))
{
    net_http2_endpoint_t http_ep = req->m_endpoint;
    net_http2_protocol_t protocol = net_http2_protocol_cast(net_endpoint_protocol(http_ep->m_base_endpoint));

    if (req->m_on_write != NULL) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: %s: req %d: set writer: already have writer!",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), http_ep->m_base_endpoint),
            net_http2_endpoint_runing_mode_str(http_ep->m_runing_mode),
            req->m_id);
        return -1;
    }
    
    req->m_write_ctx = write_ctx;
    req->m_on_write = on_write;
    req->m_write_ctx_free = write_ctx_free;

    return 0;
}

void net_http2_req_clear_writer(net_http2_req_t req) {
    req->m_write_ctx = NULL;
    req->m_on_write = NULL;
    req->m_write_ctx_free = NULL;
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
