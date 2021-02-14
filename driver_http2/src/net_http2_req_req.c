#include "net_endpoint.h"
#include "net_protocol.h"
#include "net_http2_req_i.h"

net_http2_req_method_t net_http2_req_method(net_http2_req_t req) {
    return req->m_req_method;
}

net_http2_req_state_t net_http2_req_state(net_http2_req_t req) {
    return req->m_req_state;
}

int net_http2_req_write_head_pair(net_http2_req_t http_req, const char * attr_name, const char * attr_value) {
    return 0;
}

int net_http2_req_start(net_http2_req_t http_req) {
    net_http2_endpoint_t endpoint = http_req->m_endpoint;
    net_http2_protocol_t protocol = net_protocol_data(net_endpoint_protocol(endpoint->m_base_endpoint));

    if (http_req->m_req_state != net_http2_req_state_init) {
        CPE_ERROR(
            protocol->m_em, "http2: %s: req %d: can`t start in state %s",
            net_endpoint_dump(net_http2_protocol_tmp_buffer(protocol), endpoint->m_base_endpoint),
            http_req->m_id, net_http2_req_state_str(http_req->m_req_state));
        return -1;
    }

    /* net_http2_req_state_connecting, */
    /* net_http2_req_state_established, */
    /* net_http2_req_state_error, */
    /* net_http2_req_state_done, */

    return 0;
}

const char * net_http2_req_state_str(net_http2_req_state_t req_state) {
    switch(req_state) {
    case net_http2_req_state_init:
        return "init";
    case net_http2_req_state_connecting:
        return "connecting";
    case net_http2_req_state_established:
        return "established";
    case net_http2_req_state_error:
        return "error";
    case net_http2_req_state_done:
        return "done";
    }
}
