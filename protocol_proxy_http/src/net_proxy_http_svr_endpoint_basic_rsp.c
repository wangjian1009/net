#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_endpoint_writer.h"
#include "net_proxy_http_svr_endpoint_i.h"

const char * proxy_http_svr_basic_rsp_state_str(proxy_http_svr_basic_rsp_state_t state);

static void net_proxy_http_svr_endpoint_basic_set_rsp_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_rsp_state_t rsp_state);

struct net_proxy_http_svr_endpoint_basic_rsp_head_context {
    struct net_endpoint_writer m_output;
};

static int net_proxy_http_svr_endpoint_basic_rsp_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data);

static int net_proxy_http_svr_endpoint_basic_rsp_parse_head_first(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_rsp_head_context * ctx, char * line);

static int net_proxy_http_svr_endpoint_basic_rsp_parse_head_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_rsp_head_context * ctx, char * line);

static int net_proxy_http_svr_endpoint_basic_backword_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

static int net_proxy_http_svr_endpoint_basic_backword_content_encoding_none(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from);

static int net_proxy_http_svr_endpoint_basic_backword_content_encoding_trunked(
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

        switch(http_ep->m_basic.m_rsp.m_trans_encoding) {
        case proxy_http_svr_basic_trans_encoding_none:
            if (net_proxy_http_svr_endpoint_basic_backword_content_encoding_none(http_protocol, http_ep, endpoint, from) != 0) return -1;
            break;
        case proxy_http_svr_basic_trans_encoding_trunked:
            if (net_proxy_http_svr_endpoint_basic_backword_content_encoding_trunked(http_protocol, http_ep, endpoint, from) != 0) return -1;
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
    if (net_endpoint_buf_by_str(from, net_ep_buf_forward, "\r\n\r\n", (void**)&data, &size) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: basic: search http response head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (data == NULL) {
        if(net_endpoint_buf_size(from, net_ep_buf_forward) > http_ep->m_max_head_len) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: response head too big! size=%d, max-head-len=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                net_endpoint_buf_size(from, net_ep_buf_forward),
                http_ep->m_max_head_len);
            return -1;
        }
        else {
            return 0;
        }
    }

    if (net_proxy_http_svr_endpoint_basic_rsp_read_head(http_protocol, http_ep, endpoint, data) != 0) return -1;

    net_endpoint_buf_consume(from, net_ep_buf_forward, size);

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_backword_content_encoding_none(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    if (http_ep->m_basic.m_rsp.m_content.m_length == 0) {
        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: <== ignore body no data",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        }
    }
    else {
        uint32_t forward_sz = net_endpoint_buf_size(from, net_ep_buf_forward);
        if (forward_sz == 0) return 0;
        
        if (forward_sz > http_ep->m_basic.m_rsp.m_content.m_length) {
            forward_sz = http_ep->m_basic.m_rsp.m_content.m_length;
        }

        if (net_endpoint_protocol_debug(endpoint)) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: <== body %d data(left=%d)",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz, http_ep->m_basic.m_rsp.m_content.m_length - forward_sz);

            if (net_endpoint_protocol_debug(endpoint) >= 2
                && http_ep->m_basic.m_rsp.m_content_text
                && !http_ep->m_basic.m_rsp.m_content_coded)
            {
                net_proxy_http_svr_endpoint_dump_content_text(http_protocol, from, net_ep_buf_forward, forward_sz);
            }
        }

        if (net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, forward_sz) != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: forward response content data(%d) fail",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz);
            return -1;
        }

        http_ep->m_basic.m_rsp.m_content.m_length -= forward_sz;
    }

    if (http_ep->m_basic.m_rsp.m_content.m_length == 0) {
        if (http_ep->m_basic.m_connection == proxy_http_connection_keep_alive) {
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
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_backword_content_encoding_trunked(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_endpoint_t from)
{
    uint32_t forward_sz;
    
    while((forward_sz = net_endpoint_buf_size(from, net_ep_buf_forward)) > 0) {
        if (http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_length) {
            char * data;
            uint32_t size;
            if (net_endpoint_buf_by_str(from, net_ep_buf_forward, "\r\n", (void**)&data, &size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: search trunk len fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (data == NULL) {
                if(net_endpoint_buf_size(from, net_ep_buf_forward) > 30) {
                    CPE_ERROR(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: length linke too big! size=%d",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                        net_endpoint_buf_size(from, net_ep_buf_forward));
                    return -1;
                }
                else {
                    return 0;
                }
            }
        
            char * endptr = NULL;
            http_ep->m_basic.m_rsp.m_trunked.m_length = (uint32_t)strtol(data, &endptr, 16);
            if (endptr == NULL || *endptr != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: length %s format error",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    data);
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint)) {
                CPE_INFO(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d].length = %s(%d)",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    data,
                    http_ep->m_basic.m_rsp.m_trunked.m_length);
            }

            assert(size == (uint32_t)strlen(data) + 2);
            memcpy(data + (size - 2), "\r\n", 2);
            if (net_endpoint_buf_append(endpoint, net_ep_buf_write, data, size) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: write length fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count);
                return -1;
            }

            net_endpoint_buf_consume(from, net_ep_buf_forward, size);

            http_ep->m_basic.m_rsp.m_trunked.m_state =
                http_ep->m_basic.m_rsp.m_trunked.m_length == 0
                ? proxy_http_svr_basic_trunked_complete
                : proxy_http_svr_basic_trunked_content;
        }
        else if (http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_content) {
            if (forward_sz > http_ep->m_basic.m_rsp.m_trunked.m_length) {
                forward_sz = http_ep->m_basic.m_rsp.m_trunked.m_length;
            }

            if (net_endpoint_protocol_debug(endpoint)) {
                CPE_INFO(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: %d data(left=%d)",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    forward_sz, http_ep->m_basic.m_rsp.m_trunked.m_length - forward_sz);

                if (http_ep->m_basic.m_rsp.m_content_text && !http_ep->m_basic.m_rsp.m_content_coded) {
                    net_proxy_http_svr_endpoint_dump_content_text(http_protocol, from, net_ep_buf_forward, forward_sz);
                }
            }
            
            if (net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, forward_sz) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: %d write fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count,
                    forward_sz);
                return -1;
            }

            http_ep->m_basic.m_rsp.m_trunked.m_length -= forward_sz;
            if (http_ep->m_basic.m_rsp.m_trunked.m_length == 0) {
                http_ep->m_basic.m_rsp.m_trunked.m_state = proxy_http_svr_basic_trunked_content_complete;
            }
        }
        else if (http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_content_complete) {
            if (forward_sz < 2) return 0;
            
            char * data;
            net_endpoint_buf_peak_with_size(from, net_ep_buf_forward, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: trunk rn mismatch",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count);
                return -1;
            }

            if (net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, 2) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunk[%d]: write trunk rn fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count);
                return -1;
            }
            
            http_ep->m_basic.m_rsp.m_trunked.m_state = proxy_http_svr_basic_trunked_length;
            http_ep->m_basic.m_rsp.m_trunked.m_count++;
        }
        else {
            assert(http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_complete);
            if (forward_sz < 2) return 0;
            
            char * data;
            net_endpoint_buf_peak_with_size(from, net_ep_buf_forward, 2, (void**)&data);
            assert(data);
            if (data[0] != '\r' || data[1] != '\n') {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: last rn mismach",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (net_endpoint_protocol_debug(endpoint)) {
                CPE_INFO(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: completed, trunk-count=%d",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    http_ep->m_basic.m_rsp.m_trunked.m_count);
            }

            if (net_endpoint_buf_append_from_other(endpoint, net_ep_buf_write, from, net_ep_buf_forward, 2) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: write last rn fail",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                return -1;
            }

            if (http_ep->m_basic.m_connection == proxy_http_connection_keep_alive) {
                net_proxy_http_svr_endpoint_basic_set_rsp_state(
                    http_protocol, http_ep, endpoint, proxy_http_svr_basic_rsp_state_header);
            }
            else {
                if (net_endpoint_protocol_debug(endpoint)) {
                    CPE_INFO(
                        http_protocol->m_em, "http-proxy-svr: %s: basic: <== trunked: connection is not keep-alive, close-on-send",
                        net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                }

                net_endpoint_set_close_after_send(endpoint, 1);
            }
            return 0;
        }
    }
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_rsp_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    struct net_proxy_http_svr_endpoint_basic_rsp_head_context ctx;
    bzero(&ctx, sizeof(ctx));
    net_endpoint_writer_init(&ctx.m_output, endpoint, net_ep_buf_write);

    char * line = data;
    char * sep = strstr(line, "\r\n");
    uint32_t line_num = 0;
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (line_num == 0) {
            if (net_proxy_http_svr_endpoint_basic_rsp_parse_head_first(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR;
        }
        else {
            if (net_proxy_http_svr_endpoint_basic_rsp_parse_head_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR;
        }
        line_num++;
    }

    if (line[0]) {
        if (line_num == 0) {
            if (net_proxy_http_svr_endpoint_basic_rsp_parse_head_first(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR;
        }
        else {
            if (net_proxy_http_svr_endpoint_basic_rsp_parse_head_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR;
        }
    }

    if (net_endpoint_writer_append_str(&ctx.m_output, "\r\n") != 0) goto READ_HEAD_ERROR;
    if (net_endpoint_writer_commit(&ctx.m_output) != 0) goto READ_HEAD_ERROR;

    if (net_endpoint_protocol_debug(endpoint)) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: basic: <== head(%d)",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), ctx.m_output.m_totall_len);

        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            net_proxy_http_svr_endpoint_dump_content_text(http_protocol, endpoint, net_ep_buf_write, ctx.m_output.m_totall_len - 4);
        }
    }

    net_proxy_http_svr_endpoint_basic_set_rsp_state(http_protocol, http_ep, endpoint, proxy_http_svr_basic_rsp_state_content);
    
    return 0;

READ_HEAD_ERROR:
    net_endpoint_writer_cancel(&ctx.m_output);
    return -1;
}

static void process_connection(void * ctx, const char * value) {
    net_proxy_http_svr_endpoint_t http_ep = ctx;
    if (strcasecmp(value, "Keep-Alive") == 0) {
        http_ep->m_basic.m_connection = proxy_http_connection_keep_alive;
    }
    else if (strcasecmp(value, "Close") == 0) {
        http_ep->m_basic.m_connection = proxy_http_connection_close;
    }
}

static void process_context_type(void * ctx, const char * value) {
    net_proxy_http_svr_endpoint_t http_ep = ctx;
    http_ep->m_basic.m_rsp.m_content_text = net_proxy_http_svr_endpoint_is_mine_text(value);
}

static int net_proxy_http_svr_endpoint_basic_rsp_parse_head_first(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_rsp_head_context * ctx, char * line)
{
    char * sep1 = strchr(line, ' ');
    char * sep2 = sep1 ? strchr(sep1 + 1, ' ') : NULL;
    if (sep1 == NULL || sep2 == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: rsp: parse head method: first line %s format error",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            line);
        return -1;
    }

    char * version = line;
    *sep1 = 0;
    const char * ret_code = sep1 + 1;
    *sep2 = 0;

    const char * ret_msg = sep2 + 1;

    if (net_endpoint_writer_append_str(&ctx->m_output, version) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, " ") != 0
        || net_endpoint_writer_append_str(&ctx->m_output, ret_code) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, " ") != 0
        || net_endpoint_writer_append_str(&ctx->m_output, ret_msg) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, "\r\n") != 0)
        return -1;

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_rsp_parse_head_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_rsp_head_context * ctx, char * line)
{
    char * sep = strchr(line, ':');
    if (sep == NULL) return 0;

    char * name = line;
    char * value = cpe_str_trim_head(sep + 1);

    *cpe_str_trim_tail(sep, line) = 0;

    if (strcasecmp(name, "Content-Length") == 0) {
        http_ep->m_basic.m_rsp.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        char * endptr = NULL;
        http_ep->m_basic.m_rsp.m_content.m_length = (uint32_t)strtol(value, &endptr, 10);
        if (endptr == NULL || *endptr != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: <== Content-Length %s format error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            return -1;
        }
    }
    else if (strcasecmp(name, "Connection") == 0) {
        cpe_str_list_for_each(value, ';', process_connection, http_ep);
    }
    else if (strcasecmp(name, "Content-Type") == 0) {
        cpe_str_list_for_each(value, ';', process_context_type, http_ep);
    }
    else if (strcasecmp(name, "Content-Encoding") == 0) {
        http_ep->m_basic.m_rsp.m_content_coded = 1;
    }
    else if (strcasecmp(name, "Transfer-Encoding") == 0) {
        if (strcasecmp(value, "chunked") == 0) {
            http_ep->m_basic.m_rsp.m_trans_encoding = proxy_http_svr_basic_trans_encoding_trunked;
            http_ep->m_basic.m_rsp.m_trunked.m_state = proxy_http_svr_basic_trunked_length;
            http_ep->m_basic.m_rsp.m_trunked.m_count = 0;
            http_ep->m_basic.m_rsp.m_trunked.m_length = 0;
        }
        else {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: <== Transfer-Encoding %s unknown",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
        }
    }
    
    if (net_endpoint_writer_append_str(&ctx->m_output, name) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, ": ") != 0
        || net_endpoint_writer_append_str(&ctx->m_output, value) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, "\r\n") != 0)
        return -1;

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
                http_protocol->m_em, "http-proxy-svr: %s: basic: rsp-state %s ==> %s, content-length=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp.m_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state),
                http_ep->m_basic.m_rsp.m_content.m_length);
        }
        else {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: rsp-state %s ==> %s",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_rsp_state_str(http_ep->m_basic.m_rsp.m_state),
                proxy_http_svr_basic_rsp_state_str(rsp_state));
        }
    }

    http_ep->m_basic.m_rsp.m_state = rsp_state;

    if (rsp_state == proxy_http_svr_basic_rsp_state_header) {
        http_ep->m_basic.m_rsp.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        http_ep->m_basic.m_rsp.m_content_text = 0;
        http_ep->m_basic.m_rsp.m_content_coded = 0;
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
