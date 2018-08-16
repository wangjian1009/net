#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

int net_proxy_http_svr_endpoint_tunnel_on_connect(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * request)
{
    CPE_ERROR(
        http_protocol->m_em, "http-proxy-svr: %s: connect not support yet",
        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
    return -1;
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
