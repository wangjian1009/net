#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/url.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_http_req_i.h"

static const char * net_http_req_method_str(net_http_req_method_t method);
static int net_http_req_complete_head(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t req);

net_http_req_state_t net_http_req_state(net_http_req_t req) {
    return req->m_req_state;
}

uint32_t net_http_req_head_size(net_http_req_t req) {
    return req->m_head_size;
}

uint32_t net_http_req_body_size(net_http_req_t req) {
    return req->m_body_size;
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
    return net_http_endpoint_write_head_pair(
        http_req->m_http_ep, http_req, "Host", net_address_host_inline(buf, sizeof(buf), address));
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
        net_http_req_head_tag_set(http_req, net_http_req_head_content_length);
    }
    else if (strcasecmp(attr_name, "Connection") == 0) {
        net_http_req_head_tag_set(http_req, net_http_req_head_connection);
    }
    else if (strcasecmp(attr_name, "Transfer-Encoding") == 0) {
        /* if (strcasecmp(attr_name, "chunked") == 0) { */
        /* } */
        /* else if ( */
        /*     } */
        net_http_req_head_tag_set(http_req, net_http_req_head_transfer_encoding);
    }

    return net_http_endpoint_write_head_pair(http_req->m_http_ep, http_req, attr_name, attr_value);
}

int net_http_req_write_body_full(net_http_req_t http_req, void const * data, size_t data_sz) {
    net_http_endpoint_t http_ep = http_req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_ep);

    if (http_req->m_req_state == net_http_req_state_completed) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: req-state=%s, can`t write body full!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            http_req->m_id,
            net_http_req_state_str(http_req->m_req_state));
        return -1;
    }
    
    if (http_req->m_req_state == net_http_req_state_prepare_head) {
        switch(http_req->m_req_transfer_encoding) {
        case net_http_transfer_identity:
            if (!net_http_req_head_tag_is_set(http_req, net_http_req_head_content_length)) {
                assert(http_req->m_body_size == 0);
                http_req->m_body_size = data_sz;
            }
            else {
                if (http_req->m_body_size != data_sz) {
                    CPE_ERROR(
                        http_protocol->m_em, "http: %s: req %d: head.bodysize=%d, but supply %d!",
                        net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                        http_req->m_id, http_req->m_body_size, (int)data_sz);
                    return -1;
                }
            }
            break;
        default:
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: write body: not support transform-encoding %s",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                http_req->m_id,
                net_http_transfer_encoding_str(http_req->m_req_transfer_encoding));
            return -1;
        }

        if (net_http_req_complete_head(http_protocol, http_ep, http_req) != 0) return -1;
        assert(http_req->m_req_state == net_http_req_state_prepare_body);
    }

    assert(http_req->m_req_state == net_http_req_state_prepare_body);

    if (http_req->m_body_supply_size != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: already send body %d!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
            http_req->m_id,
            http_req->m_body_supply_size);
        return -1;
    }

    switch(http_req->m_req_transfer_encoding) {
    case net_http_transfer_identity:
        if (net_http_endpoint_write(http_protocol, http_ep, http_req, data, (uint32_t)data_sz) != 0) return -1;
        http_req->m_body_supply_size = data_sz;
        break;
    case net_http_transfer_chunked:
        assert(0);
        break;
    }
    
    http_req->m_req_state = net_http_req_state_completed;
    
    return 0;
}

int net_http_req_write_commit(net_http_req_t http_req) {
    net_http_endpoint_t http_ep = http_req->m_http_ep;
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(http_req->m_http_ep);

    if (http_req->m_req_state == net_http_req_state_prepare_head) {
        if (net_http_req_complete_head(http_protocol, http_ep, http_req) != 0) return -1;
        assert(http_req->m_req_state == net_http_req_state_prepare_body);
    }

    if (http_req->m_req_state == net_http_req_state_prepare_body) {
        if (http_req->m_body_size != http_req->m_body_supply_size) {
            CPE_ERROR(
                http_protocol->m_em, "http: %s: req %d: head.bodysize=%d, but only %d!",
                net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), http_ep->m_endpoint),
                http_req->m_id, http_req->m_body_size, http_req->m_body_supply_size);
            return -1;
        }
        http_req->m_req_state = net_http_req_state_completed;
    }

    if (net_http_endpoint_flush(http_req->m_http_ep) != 0) return -1;

    return 0;
}

int net_http_req_do_send_first_line(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, const char * url)
{
    char buf[512];
    int n = snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\n", net_http_req_method_str(method), url);
    if (net_http_endpoint_write(http_protocol, http_req->m_http_ep, http_req, buf, (uint32_t)n) != 0) return -1;

    http_req->m_head_size += n;
    return 0;
}

int net_http_req_do_send_first_line_from_url(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, cpe_url_t url)
{
    char buf[512];

    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, sizeof(buf));
    stream_printf((write_stream_t)&ws, "%s ", net_http_req_method_str(method));
    cpe_url_print((write_stream_t)&ws, url, cpe_url_print_path_query);
    stream_printf((write_stream_t)&ws, " HTTP/1.1\r\n");

    if (net_http_endpoint_write(http_protocol, http_req->m_http_ep, http_req, buf, ws.m_pos) != 0) return -1;

    http_req->m_head_size += ws.m_pos;
    return 0;
}

static int net_http_req_complete_head(net_http_protocol_t http_protocol, net_http_endpoint_t http_ep, net_http_req_t http_req) {
    if (http_req->m_req_transfer_encoding == net_http_transfer_identity
        && !net_http_req_head_tag_is_set(http_req, net_http_req_head_content_length))
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", http_req->m_body_size);
        if (net_http_endpoint_write_head_pair(http_ep, http_req, "Content-Length", buf) != 0) return -1;
        net_http_req_head_tag_set(http_req, net_http_req_head_content_length);
    }
    
    if (!net_http_req_head_tag_is_set(http_req, net_http_req_head_connection)) {
        const char * connection =
            http_ep->m_connection_type == net_http_connection_type_keep_alive ? "Keep-Alive"
            : http_ep->m_connection_type == net_http_connection_type_close ? "Close"
            : "Upgrade";
        if (net_http_endpoint_write_head_pair(http_ep, http_req, "Connection", connection) != 0) return -1;
        net_http_req_head_tag_set(http_req, net_http_req_head_connection);
    }

    if (net_http_endpoint_write(http_protocol, http_ep, http_req, "\r\n", 2) != 0) return -1;
    http_req->m_head_size += 2;

    http_req->m_req_state = net_http_req_state_prepare_body;
    
    return 0;
}

static const char * net_http_req_method_str(net_http_req_method_t method) {
    switch(method) {
    case net_http_req_method_get:
        return "GET";
    case net_http_req_method_post:
        return "POST";
    case net_http_req_method_put:
        return "PUT";
    case net_http_req_method_delete:
        return "DELETE";
    case net_http_req_method_patch:
        return "PATCH";
    case net_http_req_method_head:
        return "HEAD";
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
