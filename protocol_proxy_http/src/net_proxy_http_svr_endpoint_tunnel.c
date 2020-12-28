#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_endpoint_writer.h"
#include "net_endpoint_monitor.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

static int net_proxy_http_svr_endpoint_tunnel_parse_header_method(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line);

static int net_proxy_http_svr_endpoint_tunnel_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line);

static int net_proxy_http_svr_endpoint_tunnel_do_link(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, const char * str_address);

static int net_proxy_http_svr_endpoint_tunnel_check_send_response(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t other);

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_free(void * ctx);
static void net_proxy_http_svr_endpoint_tunnel_state_monitor_process(void * ctx, net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

int net_proxy_http_svr_endpoint_tunnel_on_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    char * line = data;
    char * sep = strstr(line, "\r\n");
    uint32_t line_num = 0;
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (line_num == 0) {
            if (net_proxy_http_svr_endpoint_tunnel_parse_header_method(http_protocol, http_ep, endpoint, line) != 0) return -1;
        }
        else {
            if (net_proxy_http_svr_endpoint_tunnel_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
        }
        line_num++;
    }

    if (line[0]) {
        if (net_proxy_http_svr_endpoint_tunnel_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
    }

    /* net_endpoint_t other = net_endpoint_other(endpoint); */
    /* if (other == NULL) { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: after read head, no linked other endpoint, error!", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*     return -1; */
    /* } */

    /* if (net_proxy_http_svr_endpoint_tunnel_check_send_response(http_protocol, http_ep, endpoint, other) != 0) return -1; */

    if (http_ep->m_tunnel.m_state == proxy_http_svr_tunnel_state_connecting) {
        assert(http_ep->m_tunnel.m_other_state_monitor == NULL);

        /* http_ep->m_tunnel.m_other_state_monitor = */
        /*     net_endpoint_monitor_create( */
        /*         other, http_ep, */
        /*         net_proxy_http_svr_endpoint_tunnel_state_monitor_free, */
        /*         net_proxy_http_svr_endpoint_tunnel_state_monitor_process); */
        /* if (http_ep->m_tunnel.m_other_state_monitor == NULL) { */
        /*     CPE_ERROR( */
        /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: create other endpoint state monitor fail", */
        /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
        /*     return -1; */
        /* } */
    }
    
    return 0;
}


/* int net_proxy_http_svr_endpoint_tunnel_forward( */
/*     net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint) */
/* { */
/*     if (net_endpoint_link(endpoint) == NULL) { */
/*         CPE_ERROR( */
/*             http_protocol->m_em, "proxy-http-svr: %s: no link!", */
/*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
/*         return -1; */
/*     } */

/*     switch(net_endpoint_state(endpoint)) { */
/*     case net_endpoint_state_logic_error: */
/*         CPE_ERROR( */
/*             http_protocol->m_em, "proxy-http-svr: %s: state in error!", */
/*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
/*         return -1; */
/*     default: */
/*         break; */
/*     } */

/*     if (net_endpoint_protocol_debug(endpoint)) { */
/*         CPE_INFO( */
/*             http_protocol->m_em, "http-proxy-svr: %s: tunnel: ==> %d data", */
/*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
/*             net_endpoint_buf_size(endpoint, net_ep_buf_read)); */
/*     } */
    
/*     if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, 0) != 0) return -1; */
/*     if (net_endpoint_forward(endpoint) != 0) return -1; */
/*     return 0; */
/* } */

int net_proxy_http_svr_endpoint_tunnel_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    /* switch(http_ep->m_tunnel.m_state) { */
    /* case proxy_http_svr_tunnel_state_established: */
    /*     if (net_endpoint_protocol_debug(endpoint)) { */
    /*         CPE_INFO( */
    /*             http_protocol->m_em, "http-proxy-svr: %s: tunnel: <== %d data", */
    /*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*             net_endpoint_buf_size(from, net_ep_buf_forward)); */
    /*     } */
    
    /*     return net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, 0); */
    /* default: */
    /*     return 0; */
    /* } */
    return 0;
}

static int net_proxy_http_svr_endpoint_tunnel_parse_header_method(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line)
{
    char * sep1 = strchr(line, ' ');
    char * sep2 = sep1 ? strchr(sep1 + 1, ' ') : NULL;
    if (sep1 == NULL || sep2 == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: parse head method: first line %s format error",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            line);
        return -1;
    }

    /* char * method = line; */
    *sep1 = 0;

    char * target = sep1 + 1;
    *sep2 = 0;

    //char * version = sep2 + 1;

    char * sep = strstr(target, "://");
    if (sep) target = sep + 3;
        
    sep = strrchr(target, '/');
    if (sep) *sep = 0;

    if (net_proxy_http_svr_endpoint_tunnel_do_link(http_protocol, http_ep, endpoint, target) != 0) return -1;

    return 0;
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
        //http_ep->m_keep_alive = 1;
    }
    else if (strcasecmp(name, "HOST") == 0) {
        //TODO:
        /* if (net_endpoint_other(endpoint) == NULL) { */
        /*     if (net_proxy_http_svr_endpoint_tunnel_do_link(http_protocol, http_ep, endpoint, value) != 0) return -1; */
        /* } */
    }

    return 0;
}


static int net_proxy_http_svr_endpoint_tunnel_do_link(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, const char * str_address)
{
    net_address_t address = net_address_create_auto(net_proxy_http_svr_protocol_schedule(http_protocol), str_address);
    if (address == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: %s format error",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            str_address);
        return -1;
    }
    if (net_address_port(address) == 0) net_address_set_port(address, 80);

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: request connect to %s!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            str_address);
    }
    net_endpoint_set_remote_address(endpoint, address);

    if (http_ep->m_on_connect_fun &&
        http_ep->m_on_connect_fun(http_ep->m_on_connect_ctx, endpoint, address) != 0)
    {
        net_address_free(address);
        return -1;
    }
    net_address_free(address);

    /* net_endpoint_t other = net_endpoint_other(endpoint); */
    /* if (other == NULL) { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: connect to %s, link fail!(no other endpoint)", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*         str_address); */
    /*     return -1; */
    /* } */

    return 0;
}

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_free(void * ctx) {
    net_proxy_http_svr_endpoint_t http_ep = ctx;
    assert(http_ep->m_way == net_proxy_http_way_tunnel);
    http_ep->m_tunnel.m_other_state_monitor = NULL;
}

static void net_proxy_http_svr_endpoint_tunnel_state_monitor_process(void * ctx, net_endpoint_t other, net_endpoint_monitor_evt_t evt) {
    net_proxy_http_svr_endpoint_t http_ep = ctx;
    net_endpoint_t endpoint = http_ep->m_endpoint;
    net_proxy_http_svr_protocol_t http_protocol = net_protocol_data(net_endpoint_protocol(endpoint));

    if (evt->m_type == net_endpoint_monitor_evt_state_changed) {
        net_proxy_http_svr_endpoint_tunnel_check_send_response(http_protocol, http_ep, endpoint, other);
    }
}

static int net_proxy_http_svr_endpoint_tunnel_check_send_response(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    net_endpoint_t other)
{
    struct net_endpoint_writer output;
    net_endpoint_writer_init(&output, endpoint, net_ep_buf_write);

    switch(net_endpoint_state(other)) {
    case net_endpoint_state_disable:
    case net_endpoint_state_resolving:
    case net_endpoint_state_connecting:
        net_endpoint_writer_cancel(&output);
        return 0;
    case net_endpoint_state_established: {
        if (net_endpoint_writer_append_str(&output, "HTTP/1.1 200 Connection Established\r\n\r\n") != 0) {
            net_endpoint_writer_cancel(&output);
            return -1;
        }

        if (net_endpoint_protocol_debug(endpoint)) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: tunnel: <== first response\nHTTP/1.1 200 Connection Established",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        }
        break;
    }
    case net_endpoint_state_logic_error:
    case net_endpoint_state_network_error:
        if (net_endpoint_writer_append_str(&output, "HTTP/1.1 500 Connection Error\r\n\r\n") != 0) {
            net_endpoint_writer_cancel(&output);
            return -1;
        }

        if (net_endpoint_protocol_debug(endpoint)) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: tunnel: <== first response\nHTTP/1.1 500 Connection Error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        }
        break;
    case net_endpoint_state_deleting:
        assert(0);
        net_endpoint_writer_cancel(&output);
        return -1;
    }

    if (net_endpoint_writer_commit(&output) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: tunnel: write response error!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    http_ep->m_tunnel.m_state = proxy_http_svr_tunnel_state_established;
    if (http_ep->m_tunnel.m_other_state_monitor) {
        net_endpoint_monitor_free(http_ep->m_tunnel.m_other_state_monitor);
        http_ep->m_tunnel.m_other_state_monitor = NULL;
    }

    //TODO: Loki
    /* if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, 0) != 0 */
    /*     || net_endpoint_forward(endpoint) != 0 */
    /*     ) */
    /* { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: forward input data fail", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*     return -1; */
    /* } */

    /* if (net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, other, net_ep_buf_forward, 0) != 0) { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: tunnel: forward output data fail", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*     return -1; */
    /* } */
    
    return 0;
}
