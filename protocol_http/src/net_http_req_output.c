#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_stdio.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http_req_i.h"

static int net_http_req_complete_head(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req);

net_http_req_state_t net_http_req_state(net_http_req_t req) {
    return req->m_req_state;
}

int net_http_req_write_head_host(net_http_req_t http_req) {
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_req->m_http_ep);

    net_address_t address = net_endpoint_remote_address(http_req->m_http_ep->m_endpoint);

    if (http_req->m_req_state != net_http_req_state_prepare_head) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req-state=%s, can`t write head!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_req->m_http_ep->m_endpoint),
            http_req->m_id,
            net_http_req_state_str(http_req->m_req_state));
        return -1;
    }
    
    char buf[256];
    int n = snprintf(
        buf, sizeof(buf), "Host: %s:%d\r\n",
        net_address_host(net_http_protocol_tmp_buffer(http_protocol), address),
        net_address_port(address));

    if (net_http_endpoint_write(http_protocol, http_req->m_http_ep, http_req, buf, (uint32_t)n) != 0) return -1;

    http_req->m_head_size += n;
    return 0;
}

int net_http_req_write_head_pair(net_http_req_t http_req, const char * attr_name, const char * attr_value) {
    net_http_endpoint_t http_ep = http_req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    if (http_req->m_req_state != net_http_req_state_prepare_head) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req-state=%s, can`t write head!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            http_req->m_id,
            net_http_req_state_str(http_req->m_req_state));
        return -1;
    }
    
    if (strcasecmp(attr_name, "Content-Length") == 0) {
        http_req->m_body_size = atoi(attr_value);
    }
    else if (strcasecmp(attr_name, "Connection") == 0) {
        http_req->m_req_have_keep_alive = 1;
    }
    
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s: %s\r\n", attr_name, attr_value);

    if (net_http_endpoint_write(http_protocol, http_ep, http_req, buf, (uint32_t)n) != 0) return -1;

    http_req->m_head_size += n;
    return 0;
}

int net_http_req_write_body_full(net_http_req_t http_req, void const * data, size_t data_sz) {
    net_http_endpoint_t http_ep = http_req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    if (http_req->m_req_state != net_http_req_state_prepare_head) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req-state=%s, can`t write body full!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            http_req->m_id,
            net_http_req_state_str(http_req->m_req_state));
        return -1;
    }
    
    if (http_req->m_body_size == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", (uint32_t)data_sz);
        if (net_http_req_write_head_pair(http_req, "Content-Length", buf) != 0) return -1;
        assert(http_req->m_body_size == data_sz);
    }

    if (net_http_req_complete_head(http_protocol, http_ep, http_req) != 0) return -1;
    
    if (net_http_endpoint_write(http_protocol, http_ep, http_req, data, (uint32_t)data_sz) != 0) return -1;
    
    return 0;
}

int net_http_req_write_commit(net_http_req_t http_req) {
    net_http_endpoint_t http_ep = http_req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_req->m_http_ep);

    if (http_req->m_req_state != net_http_req_state_prepare_head
        && http_req->m_req_state != net_http_req_state_prepare_body)
    {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req-state=%s, can`t commit!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_req->m_http_ep->m_endpoint),
            http_req->m_id,
            net_http_req_state_str(http_req->m_req_state));
        return -1;
    }

    if (http_req->m_req_state == net_http_req_state_prepare_head) {
        if (net_http_req_complete_head(http_protocol, http_ep, http_req) != 0) return -1;
    }

    if (net_http_endpoint_flush(http_req->m_http_ep) != 0) return -1;

    http_req->m_req_state = net_http_req_state_completed;

    return 0;
}

int net_http_req_do_send_first_line(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, const char * url)
{
    char buf[256];
    int n = snprintf(
        buf, sizeof(buf), "%s %s HTTP/1.1\r\n",
        method == net_http_req_method_get ? "GET" : "POST",
        url);

    if (net_http_endpoint_write(http_protocol, http_req->m_http_ep, http_req, buf, (uint32_t)n) != 0) return -1;

    http_req->m_head_size += n;
    return 0;
}

static int net_http_req_complete_head(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t http_req) {
    if (!http_req->m_req_have_keep_alive) {
        if (net_http_req_write_head_pair(
                http_req, "Connection",
                http_ep->m_connection_type == net_http_connection_type_keep_alive
                ? "Keep-Alive"
                : http_ep->m_connection_type == net_http_connection_type_close
                ? "Close"
                : "Upgrade") != 0) return -1;
    }

    if (http_ep->m_request_id_tag) {
        char request_id[32];
        snprintf(request_id, sizeof(request_id), "%d", http_req->m_id);
        if (net_http_req_write_head_pair(http_req, http_ep->m_request_id_tag, request_id) != 0) return -1;
    }
    
    if (net_http_endpoint_write(http_protocol, http_ep, http_req, "\r\n", 2) != 0) return -1;
    http_req->m_head_size += 2;

    http_req->m_req_state = net_http_req_state_prepare_body;
    
    return 0;
}

void net_http_req_debug_dump_head(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t http_req, char * buf) {
    if (http_req->m_head_size >= http_req->m_flushed_size + 4) {
        char * p = buf + (http_req->m_head_size - http_req->m_flushed_size - 4);
        *p = 0;
        
        CPE_INFO(
            http_protocol->m_em, "http: %s: req %d: >>> head=%d (flushed=%d)\n%s",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_req->m_http_ep->m_endpoint),
            http_req->m_id,
            http_req->m_head_size,
            http_req->m_flushed_size,
            buf);

        *p = '\r';
    }
    else {
        CPE_INFO(
            http_protocol->m_em, "http: %s: req %d: >>> head=%d, body=%d, total=%d (flushed=%d)",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_req->m_http_ep->m_endpoint),
            http_req->m_id,
            http_req->m_head_size,
            http_req->m_body_size,
            http_req->m_head_size + http_req->m_body_size,
            http_req->m_flushed_size);
    }
}

const char * net_http_req_state_str(net_http_req_state_t req_state) {
    switch(req_state) {
    case net_http_req_state_prepare_head:
        return "http-req-prepare-head";
    case net_http_req_state_prepare_body:
        return "http-req-prepare-body";
    case net_http_req_state_completed:
        return "http-req-completed";
    }
}

