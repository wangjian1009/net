#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_proxy_http_svr_endpoint_i.h"

static int net_proxy_http_svr_endpoint_input_first_header(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);

int net_proxy_http_svr_endpoint_init(net_endpoint_t endpoint) {
    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    http_ep->m_endpoint = endpoint;
    http_ep->m_max_head_len = 8192;
    http_ep->m_way = net_proxy_http_way_unknown;
    http_ep->m_on_connect_fun = NULL;
    http_ep->m_on_connect_ctx = NULL;
    
    return 0;
}

void net_proxy_http_svr_endpoint_fini(net_endpoint_t endpoint) {
    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

    switch(http_ep->m_way) {
    case net_proxy_http_way_unknown:
        break;
    case net_proxy_http_way_tunnel:
        if (http_ep->m_tunnel.m_other_state_monitor) {
            net_endpoint_monitor_free(http_ep->m_tunnel.m_other_state_monitor);
            http_ep->m_tunnel.m_other_state_monitor = NULL;
        }
        break;
    case net_proxy_http_way_basic:
        break;
    }
}

void net_proxy_http_svr_endpoint_set_connect_fun(
    net_proxy_http_svr_endpoint_t http_ep,
    net_proxy_http_svr_connect_fun_t on_connect_fon, void * on_connect_ctx)
{
    http_ep->m_on_connect_fun = on_connect_fon;
    http_ep->m_on_connect_ctx = on_connect_ctx;
}

void * net_proxy_http_svr_endpoint_connect_ctx(net_proxy_http_svr_endpoint_t http_ep) {
    return http_ep->m_on_connect_ctx;
}

int net_proxy_http_svr_endpoint_input(net_endpoint_t endpoint) {
    net_proxy_http_svr_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

CHECK_AGAIN:
    switch(http_ep->m_way) {
    case net_proxy_http_way_unknown:
        if (net_proxy_http_svr_endpoint_input_first_header(http_protocol, http_ep, endpoint) != 0) return -1;
        if (http_ep->m_way != net_proxy_http_way_unknown) goto CHECK_AGAIN;
        break;
    case net_proxy_http_way_tunnel:
        return net_proxy_http_svr_endpoint_tunnel_forward(http_protocol, http_ep, endpoint);
    case net_proxy_http_way_basic:
        return net_proxy_http_svr_endpoint_basic_forward(http_protocol, http_ep, endpoint);
    }
    
    return 0;
}

int net_proxy_http_svr_endpoint_forward(net_endpoint_t endpoint, net_endpoint_t from) {
    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    net_proxy_http_svr_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    
    switch(http_ep->m_way) {
    case net_proxy_http_way_unknown:
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: forward on way unknown",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    case net_proxy_http_way_tunnel:
        return net_proxy_http_svr_endpoint_tunnel_backword(http_protocol, http_ep, endpoint, from);
    case net_proxy_http_way_basic:
        return net_proxy_http_svr_endpoint_basic_backword(http_protocol, http_ep, endpoint, from);
    }
}

net_proxy_http_way_t net_proxy_http_svr_endpoint_way(net_proxy_http_svr_endpoint_t http_ep) {
    return http_ep->m_way;
}

static int net_proxy_http_svr_endpoint_input_first_header(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    char * data;
    uint32_t size;
    if (net_endpoint_buf_by_str(endpoint, net_ep_buf_read, "\r\n\r\n", (void**)&data, &size) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: search http request head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (data == NULL) {
        if(net_endpoint_buf_size(endpoint, net_ep_buf_read) > http_ep->m_max_head_len) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: head too big! size=%d, max-head-len=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                net_endpoint_buf_size(endpoint, net_ep_buf_read),
                http_ep->m_max_head_len);
            return -1;
        }
        else {
            return 0;
        }
    }
        
    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: read first head",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        
        net_proxy_http_svr_endpoint_dump_content_text(http_protocol, endpoint, net_ep_buf_read, size);
    }

    int rv;
    
    if (cpe_str_start_with(data, "CONNECT")) {
        http_ep->m_way = net_proxy_http_way_tunnel;
        http_ep->m_tunnel.m_state = proxy_http_svr_tunnel_state_connecting;
        http_ep->m_tunnel.m_other_state_monitor = NULL;
        rv = net_proxy_http_svr_endpoint_tunnel_on_connect(http_protocol, http_ep, endpoint, data);
    }
    else {
        http_ep->m_way = net_proxy_http_way_basic;
        http_ep->m_basic.m_version = proxy_http_version_unknown;
        http_ep->m_basic.m_connection = proxy_http_connection_unknown;
        http_ep->m_basic.m_req.m_state = proxy_http_svr_basic_req_state_header;
        http_ep->m_basic.m_req.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        http_ep->m_basic.m_req.m_content_text = 0;
        http_ep->m_basic.m_req.m_content_coded = 0;
        http_ep->m_basic.m_req.m_content.m_length = 0;
        http_ep->m_basic.m_rsp.m_state = proxy_http_svr_basic_rsp_state_header;
        http_ep->m_basic.m_rsp.m_content_text = 0;
        http_ep->m_basic.m_rsp.m_content_coded = 0;
        http_ep->m_basic.m_rsp.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        http_ep->m_basic.m_rsp.m_content.m_length = 0;
        rv = net_proxy_http_svr_endpoint_basic_req_read_head(http_protocol, http_ep, endpoint, data);
    }
    
    net_endpoint_buf_consume(endpoint, net_ep_buf_read, size);

    return rv;
}

int net_proxy_http_svr_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state) {
    if (net_endpoint_is_active(endpoint)) return 0;

    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);
    
    switch(http_ep->m_way) {
    case net_proxy_http_way_unknown:
        break;
    case net_proxy_http_way_tunnel:
        if (http_ep->m_tunnel.m_other_state_monitor) {
            net_endpoint_monitor_free(http_ep->m_tunnel.m_other_state_monitor);
            http_ep->m_tunnel.m_other_state_monitor = NULL;
        }
        break;
    case net_proxy_http_way_basic:
        break;
    }
    
    return -1;
}

uint8_t net_proxy_http_svr_endpoint_is_mine_text(const char * mine) {
    if (cpe_str_start_with(mine, "text/")) {
        return 1;
    }

    return 0;
}

void net_proxy_http_svr_endpoint_dump_line(net_proxy_http_svr_protocol_t http_protocol, const char * prefix, const char * data, uint32_t data_sz) {
    uint8_t line_num = 0;
    do {
        uint32_t line_sz = data_sz;
        if (line_sz > 100) line_sz = 100;

        if (line_num == 0) {
            CPE_INFO(http_protocol->m_em, "%s%.*s", prefix, line_sz, data);
        }
        else {
            CPE_INFO(http_protocol->m_em, "%s+ %d: %.*s", prefix, line_num, line_sz, data);
        }
        line_num++;

        data_sz -= line_sz;
        data += line_sz;
    } while(data_sz > 0);
}
    
void net_proxy_http_svr_endpoint_dump_content_text(
    net_proxy_http_svr_protocol_t http_protocol,
    net_endpoint_t endpoint, net_endpoint_buf_type_t buf, uint32_t data_sz)
{
    char * data;
    if (net_endpoint_buf_peak_with_size(endpoint, buf, data_sz, (void**)&data) != 0 || data == NULL) {
        CPE_ERROR(http_protocol->m_em, "    read context error");
    }
    else {
        char * sep;
        while(data_sz > 0 && (sep = memchr(data, '\r', data_sz)) != NULL) {
            uint32_t line_sz = (uint32_t)(sep - data);
            net_proxy_http_svr_endpoint_dump_line(http_protocol, "    ", data, line_sz);

            data_sz -= line_sz + 1;
            data += line_sz + 1;

            if (data_sz > 0 && data[0] == '\n') {
                data++;
                data_sz--;
            }
        }

        if (data_sz > 0) {
            CPE_INFO(http_protocol->m_em, "    %.*s", data_sz, data);
        }
    }
}
