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

static int net_proxy_http_svr_endpoint_basic_rsp_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data);

static int net_proxy_http_svr_endpoint_basic_rsp_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line);

static int net_proxy_http_svr_endpoint_basic_backword_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

static int net_proxy_http_svr_endpoint_basic_backword_body_content(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

static int net_proxy_http_svr_endpoint_basic_backword_body_trunk(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

int net_proxy_http_svr_endpoint_basic_backword(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
CHECK_AGAIN:
    if (http_ep->m_basic.m_rsp.m_state == proxy_http_svr_basic_rsp_state_header) {
        if (net_proxy_http_svr_endpoint_basic_backword_head(http_protocol, http_ep, endpoint, from) != 0) return -1;
        if (http_ep->m_basic.m_rsp.m_state != proxy_http_svr_basic_rsp_state_header) goto CHECK_AGAIN;
    }
    else {
        assert(http_ep->m_basic.m_rsp.m_state == proxy_http_svr_basic_rsp_state_content);

        switch(http_ep->m_basic.m_rsp.m_trans_way) {
        case proxy_http_svr_basic_rsp_trans_content:
            if (net_proxy_http_svr_endpoint_basic_backword_body_content(http_protocol, http_ep, endpoint, from) != 0) return -1;
            break;
        case proxy_http_svr_basic_rsp_trans_trunked:
            if (net_proxy_http_svr_endpoint_basic_backword_body_trunk(http_protocol, http_ep, endpoint, from) != 0) return -1;
            break;
        }

        if (http_ep->m_basic.m_rsp.m_state != proxy_http_svr_basic_rsp_state_content) goto CHECK_AGAIN;
    }

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_backword_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    char * data;
    uint32_t size;
    if (net_endpoint_fbuf_by_str(from, "\r\n\r\n", (void**)&data, &size) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: basic: search http response head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (data == NULL) {
        if(net_endpoint_fbuf_size(from) > http_ep->m_max_head_len) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: response head too big! size=%d, max-head-len=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                net_endpoint_fbuf_size(from),
                http_ep->m_max_head_len);
            return -1;
        }
        else {
            return 0;
        }
    }

    if (net_proxy_http_svr_endpoint_basic_rsp_read_head(http_protocol, http_ep, endpoint, data) != 0) return -1;

    net_endpoint_fbuf_consume(from, size);

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_backword_body_content(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    uint32_t forward_sz = net_endpoint_fbuf_size(from);
    if (forward_sz == 0) return 0;
    
    if (http_ep->m_basic.m_rsp.m_content.m_length == 0) {
        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: <== body %d data",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz);
        }
    
        if (net_endpoint_wbuf_append_from_other(endpoint, from, 0) != 0
            || net_endpoint_forward(endpoint) != 0
            )
        {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: backward rsp content data fail",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
            return -1;
        }
    }
    else {
        if (forward_sz > http_ep->m_basic.m_rsp.m_content.m_length) {
            forward_sz = http_ep->m_basic.m_rsp.m_content.m_length;
        }

        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: ==> body %d data(left=%d)",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz, http_ep->m_basic.m_rsp.m_content.m_length - forward_sz);
        }

        if (net_endpoint_wbuf_append_from_other(endpoint, from, forward_sz) != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: forward response content data(%d) fail",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz);
            return -1;
        }

        http_ep->m_basic.m_rsp.m_content.m_length -= forward_sz;
        if (http_ep->m_basic.m_rsp.m_content.m_length == 0) {
            if (http_ep->m_keep_alive) {
                net_proxy_http_svr_endpoint_basic_set_rsp_state(
                    http_protocol, http_ep, endpoint, proxy_http_svr_basic_rsp_state_header);
            }
            else {
                if (net_endpoint_protocol_debug(endpoint)) {
                    CPE_INFO(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: connection is not keep-alive, close-on-send",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                }

                net_endpoint_set_close_after_send(endpoint, 1);
            }
        }
    }

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_backword_body_trunk(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    uint32_t forward_sz;
    
    while((forward_sz = net_endpoint_fbuf_size(from)) > 0) {
        if (http_ep->m_basic.m_rsp.m_trunked.m_length == 0) { /*读取length */
            char * data;
            uint32_t size;
            if (net_endpoint_fbuf_by_str(from, "\r\n", (void**)&data, &size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: trunked: search trunk len fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (data == NULL) {
                if(net_endpoint_fbuf_size(from) > 30) {
                    CPE_ERROR(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: trunked: length linke too big! size=%d",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                        net_endpoint_fbuf_size(from));
                    return -1;
                }
                else {
                    return 0;
                }
            }

            if (data[0] == 0) { /*结速行 */
                if (net_endpoint_protocol_debug(endpoint) >= 2) {
                    CPE_INFO(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: trunked transfer completed, total count = %d",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                        http_ep->m_basic.m_rsp.m_trunked.m_count);
                }

                if (net_endpoint_wbuf_append(endpoint, "\r\n", 2) != 0) {
                    CPE_ERROR(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: write last sep fail",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                    return -1;
                }

                net_endpoint_fbuf_consume(from, 2);
                
                if (http_ep->m_keep_alive) {
                    net_proxy_http_svr_endpoint_basic_set_rsp_state(
                        http_protocol, http_ep, endpoint, proxy_http_svr_basic_rsp_state_header);
                }
                else {
                    if (net_endpoint_protocol_debug(endpoint)) {
                        CPE_INFO(
                            http_protocol->m_em, "http-proxy-svr: %s: basic: connection is not keep-alive, close-on-send",
                            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                    }

                    net_endpoint_set_close_after_send(endpoint, 1);
                }
                return 0;
            }
        
            char * endptr = NULL;
            http_ep->m_basic.m_rsp.m_trunked.m_length = (uint32_t)strtol(data, &endptr, 16);
            if (endptr == NULL || *endptr != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: trunk-length %s format error",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    data);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: <== trunk[%d].length = %s(%d)",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    data,
                    http_ep->m_basic.m_rsp.m_trunked.m_length);
            }

            assert(size == (uint32_t)strlen(data) + 2);
            memcpy(data + (size - 2), "\r\n", 2);
            if (net_endpoint_wbuf_append(endpoint, data, size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: write trunk length fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            net_endpoint_fbuf_consume(from, size);
        }
        else {
            if (forward_sz > http_ep->m_basic.m_rsp.m_trunked.m_length) {
                forward_sz = http_ep->m_basic.m_rsp.m_trunked.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint) >= 2) {
                CPE_INFO(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunk[%d]: %d data(left=%d)",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    forward_sz, http_ep->m_basic.m_rsp.m_trunked.m_length - forward_sz);
            }

            if (net_endpoint_wbuf_append_from_other(endpoint, from, forward_sz) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: forward response trunk %d data(%d) fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    forward_sz);
                return -1;
            }

            http_ep->m_basic.m_rsp.m_trunked.m_length -= forward_sz;
            if (http_ep->m_basic.m_rsp.m_trunked.m_length == 0) {
                http_ep->m_basic.m_rsp.m_trunked.m_count++;
            }
        }
    }
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_rsp_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    char * line = data;
    char * sep = strstr(line, "\r\n");
    uint32_t line_num = 0;
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_proxy_http_svr_endpoint_basic_rsp_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
        *sep = '\r';
        line_num++;
    }

    if (line[0]) {
        if (net_proxy_http_svr_endpoint_basic_rsp_parse_header_line(http_protocol, http_ep, endpoint, line) != 0) return -1;
    }

    if (net_endpoint_protocol_debug(endpoint) >= 2) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: basic: <== head\n%s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            data);
    }

    uint32_t data_len =  (uint32_t)strlen(data);
    memcpy(data + data_len, "\r\n\r\n", 4);
    data_len += 4;
    if (net_endpoint_wbuf_append(endpoint, data, data_len) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: basic: write response head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    net_proxy_http_svr_endpoint_basic_set_rsp_state(http_protocol, http_ep, endpoint, proxy_http_svr_basic_rsp_state_content);
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_rsp_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * line)
{
    char * sep = strchr(line, ':');
    if (sep == NULL) return 0;

    char * name = line;
    char * value = cpe_str_trim_head(sep + 1);

    sep = cpe_str_trim_tail(sep, line);

    char keep = *sep;
    *sep = 0;

    if (strcasecmp(name, "Content-Length") == 0) {
        http_ep->m_basic.m_rsp.m_trans_way = proxy_http_svr_basic_rsp_trans_content;
        char * endptr = NULL;
        http_ep->m_basic.m_rsp.m_content.m_length = (uint32_t)strtol(value, &endptr, 10);
        if (endptr == NULL || *endptr != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: Content-Length %s format error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            *sep = keep;
            return -1;
        }
    }
    else if (strcasecmp(name, "Transfer-Encoding") == 0) {
        if (strcasecmp(value, "chunked") == 0) {
            http_ep->m_basic.m_rsp.m_trans_way = proxy_http_svr_basic_rsp_trans_trunked;
            http_ep->m_basic.m_rsp.m_trunked.m_count = 0;
            http_ep->m_basic.m_rsp.m_trunked.m_length = 0;
        }
        else {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: rsp: Transfer-Encoding %s unknown",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
        }
    }
    
    *sep = keep;
    return 0;
}

static void net_proxy_http_svr_endpoint_basic_set_rsp_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_rsp_state_t rsp_state)
{
    if (http_ep->m_basic.m_rsp.m_state == rsp_state) return;

    if (net_endpoint_protocol_debug(endpoint)) {
        if (rsp_state == proxy_http_svr_basic_rsp_state_content) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: rsp-state %s ==> %s, content-length=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp.m_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state),
                http_ep->m_basic.m_rsp.m_content.m_length);
        }
        else {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: rsp-state %s ==> %s",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp.m_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state));
        }
    }

    http_ep->m_basic.m_rsp.m_state = rsp_state;

    if (rsp_state == proxy_http_svr_basic_rsp_state_header) {
        http_ep->m_basic.m_rsp.m_trans_way = proxy_http_svr_basic_rsp_trans_content;
        http_ep->m_basic.m_rsp.m_content.m_length = 0;
    }
}

const char * proxy_http_svr_basic_rsp_state_str(proxy_http_svr_basic_rsp_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_rsp_state_header:
        return "rsp-header";
    case proxy_http_svr_basic_rsp_state_content:
        return "rsp-content";
    }
}
