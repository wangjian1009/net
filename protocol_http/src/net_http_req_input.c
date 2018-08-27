#include "net_endpoint.h"
#include "net_http_req_i.h"

int net_http_req_set_reader(
    net_http_req_t req,
    void * ctx,
    net_http_req_on_res_begin_fun_t on_begin,
    net_http_req_on_res_head_fun_t on_head,
    net_http_req_on_res_body_fun_t on_body,
    net_http_req_on_res_complete_fun_t on_complete)
{
    net_http_protocol_t http_protocol = net_http_endpoint_protocol(req->m_http_ep);
    
    if (req->m_res_state != net_http_res_state_init) {
        CPE_ERROR(
            http_protocol->m_em, "http: %s: req %d: res-state=%s, can`t set reader!",
            net_endpoint_dump(net_http_protocol_tmp_buffer(http_protocol), req->m_http_ep->m_endpoint),
            req->m_id,
            net_http_res_state_str(req->m_res_state));
        return -1;
    }

    req->m_res_ctx = ctx;
    req->m_res_on_begin = on_begin;
    req->m_res_on_head = on_head;
    req->m_res_on_body = on_body;
    req->m_res_on_complete = on_complete;
    
    return 0;
}

int net_http_endpoint_req_input(net_http_protocol_t ws_protocol, net_http_req_t req) {
    switch(req->m_res_state) {
    case net_http_res_state_init:
    case net_http_res_state_reading_head:
    case net_http_res_state_reading_body:
    case net_http_res_state_completed:
        break;
    }
    
    return 0;
}

const char * net_http_res_state_str(net_http_res_state_t res_state) {
    switch(res_state) {
    case net_http_res_state_init:
        return "http-res-init"; 
    case net_http_res_state_reading_head:
        return "http-res-reading-head";
    case net_http_res_state_reading_body:
        return "http-res-reading-body";
    case net_http_res_state_completed:
        return "http-res-completed";
    }
}

    /* char * sep = strchr(line, ':'); */
    /* if (sep == NULL) return 0; */

    /* char * name = line; */
    /* char * value = cpe_str_trim_head(sep + 1); */

    /* *cpe_str_trim_tail(sep, line) = 0; */
    
    /* if (strcasecmp(name, "sec-websocket-accept") == 0) { */
    /*     char accept_token_buf[64]; */
    /*     snprintf(accept_token_buf, sizeof(accept_token_buf), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_ep->m_handshake_token); */

    /*     struct cpe_sha1_value sha1_value; */
    /*     if (cpe_sha1_encode_str(&sha1_value, accept_token_buf) != 0) { */
    /*         CPE_ERROR( */
    /*             ws_protocol->m_em, "ws: %s: handshake response: calc accept token fail", */
    /*             net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep))); */
    /*         return -1; */
    /*     } */

    /*     const char * expect_accept = cpe_base64_dump(net_ws_cli_protocol_tmp_buffer(ws_protocol), sha1_value.digest, sizeof(sha1_value.digest)); */
    /*     if (strcmp(value, expect_accept) != 0) { */
    /*         CPE_ERROR( */
    /*             ws_protocol->m_em, "ws: %s: handshake response: expect token %s, but %s", */
    /*             net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(ws_protocol), net_http_endpoint_net_ep(ws_ep->m_http_ep)), */
    /*             expect_accept, value); */
    /*         return -1; */
    /*     } */

    /*     if (net_ws_cli_endpoint_set_state(ws_ep, net_ws_cli_state_established) != 0) return -1; */
    /* } */
    
    /* return 0; */
