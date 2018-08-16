#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_endpoint_monitor.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

static int net_proxy_http_svr_endpoint_tunnel_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line);

static int net_proxy_http_svr_endpoint_tunnel_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, const char * str_address);

static int net_proxy_http_svr_endpoint_tunnel_check_send_response(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t other);

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_free(void * ctx);
static void net_proxy_http_svr_endpoint_tunnel_state_monitor_process(void * ctx, net_endpoint_t endpoint, net_endpoint_state_t from_state);

int net_proxy_http_svr_endpoint_tunnel_on_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    char * line = data;
    char * sep = strstr(line, "\r\n");
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_proxy_http_svr_endpoint_tunnel_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
        *sep = '\r';
    }

    if (line[0]) {
        if (net_proxy_http_svr_endpoint_tunnel_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
    }

    net_endpoint_t other = net_endpoint_other(endpoint);
    if (other == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: after read head, no linked other endpoint, error!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (net_proxy_http_svr_endpoint_tunnel_check_send_response(http_protocol, http_ep, endpoint, other) != 0) return -1;

    if (http_ep->m_tunnel.m_state == proxy_http_svr_tunnel_state_connecting) {
        assert(http_ep->m_tunnel.m_other_state_monitor == NULL);

        http_ep->m_tunnel.m_other_state_monitor =
            net_endpoint_monitor_create(
                other, http_ep,
                net_proxy_http_svr_endpoint_tunnel_state_monitor_free,
                net_proxy_http_svr_endpoint_tunnel_state_monitor_process);
        if (http_ep->m_tunnel.m_other_state_monitor == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: tunnel: create other endpoint state monitor fail",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
            return -1;
        }

        /* if (http_ep->m_debug) { */
        /*     CPE_INFO( */
        /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: create other endpoint state monitor created", */
        /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
        /* } */
    }
    
    return 0;
}


int net_proxy_http_svr_endpoint_tunnel_forward(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    if (net_endpoint_link(endpoint) == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "proxy-http-svr: %s: no link!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    switch(net_endpoint_state(endpoint)) {
    case net_endpoint_state_logic_error:
        CPE_ERROR(
            http_protocol->m_em, "proxy-http-svr: %s: state in error!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    default:
        break;
    }

    if (net_endpoint_fbuf_append_from_rbuf(endpoint, 0) != 0) return -1;
    if (net_endpoint_forward(endpoint) != 0) return -1;
    return 0;
}

int net_proxy_http_svr_endpoint_tunnel_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    return net_endpoint_wbuf_append_from_other(endpoint, from, 0);
}

static int net_proxy_http_svr_endpoint_tunnel_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line)
{
    char * sep = strchr(line, ':');
    if (sep == NULL) return 0;

    char * name = line;
    char * value = cpe_str_trim_head(sep + 1);

    *cpe_str_trim_tail(sep, line) = 0;

    if (strcasecmp(name, "Proxy-Authenticate") == 0) {
    }
    else if (strcasecmp(name, "Proxy-Authorization") == 0) {
    }
    else if (strcasecmp(name, "Proxy-Connection") == 0) {
        http_ep->m_keep_alive = 1;
    }
    else if (strcasecmp(name, "HOST") == 0) {
        if (net_endpoint_other(endpoint) == NULL) {
            if (net_proxy_http_svr_endpoint_tunnel_connect(http_protocol, http_ep, endpoint, value) != 0) return -1;
        }
    }
    else if (strcasecmp(name, "CONNECT") == 0) {
        if (net_endpoint_other(endpoint) == NULL) {
            sep = strchr(value, ':');
            if (sep) value = sep + 1;
        
            sep = strchr(value, ' ');
            if (sep) *sep = 0;

            sep = strrchr(value, '/');
            if (sep) *sep = 0;

            if (net_proxy_http_svr_endpoint_tunnel_connect(http_protocol, http_ep, endpoint, value) != 0) return -1;
        }
    }

    return 0;
}


static int net_proxy_http_svr_endpoint_tunnel_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, const char * str_address)
{
    net_address_t address = net_address_create_auto(net_proxy_http_svr_protocol_schedule(http_protocol), str_address);
    if (address == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: Host %s format error",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            str_address);
        return -1;
    }
    if (net_address_port(address) == 0) net_address_set_port(address, 80);

    if (http_ep->m_debug) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: request connect to %s!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            str_address);
    }

    if (http_ep->m_on_connect_fun &&
        http_ep->m_on_connect_fun(http_ep->m_on_connect_ctx, endpoint, address, 1) != 0)
    {
        net_address_free(address);
        return -1;
    }

    net_endpoint_t other = net_endpoint_other(endpoint);
    if (other == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: connect to %s, link fail!(no other endpoint)",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            str_address);
        return -1;
    }

    return 0;
}

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_free(void * ctx) {
    net_proxy_http_svr_endpoint_t http_ep = ctx;
    assert(http_ep->m_way == net_proxy_http_way_tunnel);
    http_ep->m_tunnel.m_other_state_monitor = NULL;
}

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_process(void * ctx, net_endpoint_t other, net_endpoint_state_t from_state) {
    net_endpoint_t endpoint = net_endpoint_other(other);
    net_proxy_http_svr_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));
    net_proxy_http_svr_endpoint_t http_ep = ctx;

    net_proxy_http_svr_endpoint_tunnel_check_send_response(http_protocol, http_ep, endpoint, other);
}

static int net_proxy_http_svr_endpoint_tunnel_check_send_response(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    net_endpoint_t other)
{
    mem_buffer_clear_data(&http_protocol->m_data_buffer);
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(&http_protocol->m_data_buffer);
    
    switch(net_endpoint_state(other)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        return 0;
    case net_endpoint_state_established: {
        stream_printf((write_stream_t)&ws, "HTTP/1.1 200 Connection Established\n\n");
        break;
    }
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
        stream_printf((write_stream_t)&ws, "HTTP/1.1 500 Connection Error\n\n");
        break;
    }

    mem_buffer_append_char(&http_protocol->m_data_buffer, 0);

    char * response = mem_buffer_make_continuous(&http_protocol->m_data_buffer, 0);

    if (net_endpoint_wbuf_append(endpoint, response, (uint32_t)mem_buffer_size(&http_protocol->m_data_buffer) - 1u) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: write response error\n%s!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            response);
        return -1;
    }

    if (http_ep->m_debug) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: --> %s!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            response);
    }

    http_ep->m_tunnel.m_state = proxy_http_svr_tunnel_state_established;
    if (http_ep->m_tunnel.m_other_state_monitor) {
        net_endpoint_monitor_free(http_ep->m_tunnel.m_other_state_monitor);
        http_ep->m_tunnel.m_other_state_monitor = NULL;
    }

    if (net_endpoint_fbuf_append_from_rbuf(endpoint, 0) != 0
        || net_endpoint_forward(endpoint) != 0
        )
    {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: forward input data fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (net_endpoint_wbuf_append_from_other(endpoint, other, 0) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: forward output data fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }
    
    return 0;
}
