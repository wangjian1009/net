#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

const char * proxy_http_svr_basic_rsp_state_str(proxy_http_svr_basic_rsp_state_t state);

static void net_proxy_http_svr_endpoint_basic_set_rsp_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_rsp_state_t rsp_state);

int net_proxy_http_svr_endpoint_basic_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    /* if (http_ep->m_basic.m__state == proxy_http_svr_basic_req_state_header) { */
    /*     char * data; */
    /*     uint32_t size; */
    /*     if (net_endpoint_fbuf_by_str(endpoint, "\r\n\r\n", (void**)&data, &size) != 0) { */
    /*         CPE_ERROR( */
    /*             http_protocol->m_em, "http-proxy-svr: %s: basic: search http request head fail", */
    /*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*         return -1; */
    /*     } */

    /*     if (data == NULL) { */
    /*         if(net_endpoint_rbuf_size(endpoint) > http_ep->m_max_head_len) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: head too big! size=%d, max-head-len=%d", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 net_endpoint_rbuf_size(endpoint), */
    /*                 http_ep->m_max_head_len); */
    /*             return -1; */
    /*         } */
    /*         else { */
    /*             return 0; */
    /*         } */
    /*     } */

    /* /\* if (net_endpoint_protocol_debug(endpoint) >= 2) { *\/ */
    /*     CPE_INFO( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: basic: <== %d data", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*         net_endpoint_fbuf_size(from)); */
    /* /\* } *\/ */
    
    return net_endpoint_wbuf_append_from_other(endpoint, from, 0);
}

static void net_proxy_http_svr_endpoint_basic_set_rsp_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_rsp_state_t rsp_state)
{
    if (http_ep->m_basic.m_rsp_state == rsp_state) return;

    if (net_endpoint_protocol_debug(endpoint)) {
        if (rsp_state == proxy_http_svr_basic_rsp_state_content) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: write-state %s ==> %s, content-length=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state),
                http_ep->m_basic.m_rsp_context_length);
        }
        else {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: write-state %s ==> %s",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state));
        }
    }

    http_ep->m_basic.m_rsp_state = rsp_state;
}

const char * proxy_http_svr_basic_rsp_state_str(proxy_http_svr_basic_rsp_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_rsp_state_header:
        return "rsp-header";
    case proxy_http_svr_basic_rsp_state_content:
        return "rsp-content";
    }
}
