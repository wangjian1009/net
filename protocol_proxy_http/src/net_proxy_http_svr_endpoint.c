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
    http_ep->m_debug = 0;
    http_ep->m_way = net_proxy_http_way_unknown;
    http_ep->m_keep_alive = 0;

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

int net_proxy_http_svr_endpoint_input(net_endpoint_t endpoint) {
    net_proxy_http_svr_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_proxy_http_svr_endpoint_t http_ep = net_endpoint_protocol_data(endpoint);

CHECK_AGAIN:
    switch(http_ep->m_way) {
    case net_proxy_http_way_unknown:
        if (net_proxy_http_svr_endpoint_input_first_header(http_protocol, http_ep, endpoint) != 0) return -1;
        if (http_ep->m_way != http_ep->m_way) goto CHECK_AGAIN;
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

uint8_t net_proxy_http_svr_endpoint_debug(net_proxy_http_svr_endpoint_t http_ep) {
    return http_ep->m_debug;
}

void net_proxy_http_svr_endpoint_set_debug(net_proxy_http_svr_endpoint_t http_ep, uint8_t debug) {
    http_ep->m_debug = debug;
}

static int net_proxy_http_svr_endpoint_input_first_header(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    char * data;
    uint32_t size;
    if (net_endpoint_rbuf_by_str(endpoint, "\r\n\r\n", (void**)&data, &size) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: search http request head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (data == NULL) {
        if(net_endpoint_rbuf_size(endpoint) > 8192) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: first head Too big response head!, size=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                net_endpoint_rbuf_size(endpoint));
            return -1;
        }
        else {
            return 0;
        }
    }
        
    if (http_ep->m_debug) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: read first head\n%s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            data);
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
        http_ep->m_basic.m_read_state = proxy_http_svr_basic_state_reading_header;
        http_ep->m_basic.m_read_context_length = 0;
        rv = net_proxy_http_svr_endpoint_basic_read_head(http_protocol, http_ep, endpoint, data);
    }
    
    net_endpoint_rbuf_consume(endpoint, size);

    return rv;
}

