#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_endpoint_writer.h"
#include "net_proxy_http_svr_endpoint_i.h"

const char * proxy_http_svr_basic_req_state_str(proxy_http_svr_basic_req_state_t state);

static void net_proxy_http_svr_endpoint_basic_req_set_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_req_state_t req_state);

struct net_proxy_http_svr_endpoint_basic_req_head_context {
    uint8_t m_header_has_connection;
    const char * m_method;
    net_address_t m_host;
    struct net_endpoint_writer m_output;
};

static int net_proxy_http_svr_endpoint_basic_req_parse_header_method(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_req_head_context * ctx, char * line);

static int net_proxy_http_svr_endpoint_basic_req_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_req_head_context * ctx, char * line);

static int net_proxy_http_svr_endpoint_basic_forward_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);

static int net_proxy_http_svr_endpoint_basic_forward_direct_remote(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    net_address_t host);

static int net_proxy_http_svr_endpoint_basic_forward_content_encoding_none(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);

static int net_proxy_http_svr_endpoint_basic_forward_content_encoding_trunked(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint);

int net_proxy_http_svr_endpoint_basic_forward(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
CHECK_AGAIN:
    if (http_ep->m_basic.m_req.m_state == proxy_http_svr_basic_req_state_header) {
        if (net_proxy_http_svr_endpoint_basic_forward_head(http_protocol, http_ep, endpoint) != 0) return -1;
        if (http_ep->m_basic.m_req.m_state != proxy_http_svr_basic_req_state_header) goto CHECK_AGAIN;
    }
    else if (http_ep->m_basic.m_req.m_state == proxy_http_svr_basic_req_state_content) {
        switch(http_ep->m_basic.m_req.m_trans_encoding) {
        case proxy_http_svr_basic_trans_encoding_none:
            if (net_proxy_http_svr_endpoint_basic_forward_content_encoding_none(http_protocol, http_ep, endpoint) != 0) return -1;
            break;
        case proxy_http_svr_basic_trans_encoding_trunked:
            if (net_proxy_http_svr_endpoint_basic_forward_content_encoding_trunked(http_protocol, http_ep, endpoint) != 0) return -1;
            break;
        }
        
        if (http_ep->m_basic.m_req.m_state != proxy_http_svr_basic_req_state_content) goto CHECK_AGAIN;
    }
    else {
        assert(http_ep->m_basic.m_req.m_state == proxy_http_svr_basic_req_state_stop);

        if(net_endpoint_buf_size(endpoint, net_ep_buf_read) > 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: reading is stoped, still incoming data",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
            return -1;
        }
    }
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_forward_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    char * data;
    uint32_t size;
    if (net_endpoint_buf_by_str(endpoint, net_ep_buf_read, "\r\n\r\n", (void**)&data, &size) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: basic: search http request head fail",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (data == NULL) {
        if(net_endpoint_buf_size(endpoint, net_ep_buf_read) > http_ep->m_max_head_len) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: basic: head too big! size=%d, max-head-len=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                net_endpoint_buf_size(endpoint, net_ep_buf_read),
                http_ep->m_max_head_len);
            return -1;
        }
        else {
            return 0;
        }
    }

    if (net_proxy_http_svr_endpoint_basic_req_read_head(http_protocol, http_ep, endpoint, data) != 0) return -1;

    net_endpoint_buf_consume(endpoint, net_ep_buf_read, size);

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_forward_content_encoding_none(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    if (http_ep->m_basic.m_req.m_content.m_length == 0) {
        if (net_endpoint_protocol_debug(endpoint) >= 2) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: ==> skip empty content",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        }
    
        net_proxy_http_svr_endpoint_basic_req_set_state(
            http_protocol, http_ep, endpoint,
            http_ep->m_basic.m_connection == proxy_http_connection_keep_alive
            ? proxy_http_svr_basic_req_state_header
            : proxy_http_svr_basic_req_state_stop);
    }
    else {
        uint32_t forward_sz = net_endpoint_buf_size(endpoint, net_ep_buf_read);
        if (forward_sz == 0) return 0;
        
        if (forward_sz > http_ep->m_basic.m_req.m_content.m_length) {
            forward_sz = http_ep->m_basic.m_req.m_content.m_length;
        }

        if (net_endpoint_protocol_debug(endpoint)) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: ==> content %d data(left=%d)",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                forward_sz, http_ep->m_basic.m_req.m_content.m_length - forward_sz);

            if (net_endpoint_protocol_debug(endpoint) >= 2
                && http_ep->m_basic.m_req.m_content_text && !http_ep->m_basic.m_req.m_content_coded)
            {
                net_proxy_http_svr_endpoint_dump_content_text(http_protocol, endpoint, net_ep_buf_read, forward_sz);
            }
        }

        //TODO: Loki
        /* if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, forward_sz) != 0 */
        /*     || net_endpoint_forward(endpoint) != 0 */
        /*     ) */
        /* { */
        /*     CPE_ERROR( */
        /*         http_protocol->m_em, "http-proxy-svr: %s: basic: forward content data(%d) fail", */
        /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
        /*         forward_sz); */
        /*     return -1; */
        /* } */

        http_ep->m_basic.m_req.m_content.m_length -= forward_sz;
        if (http_ep->m_basic.m_req.m_content.m_length == 0) {
            net_proxy_http_svr_endpoint_basic_req_set_state(
                http_protocol, http_ep, endpoint,
                http_ep->m_basic.m_connection == proxy_http_connection_keep_alive
                ? proxy_http_svr_basic_req_state_header
                : proxy_http_svr_basic_req_state_stop);
        }
    }

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_forward_content_encoding_trunked(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint)
{
    //TODO: Loki
    /* uint32_t forward_sz; */
    
    /* while((forward_sz = net_endpoint_buf_size(endpoint, net_ep_buf_read)) > 0) { */
    /*     if (http_ep->m_basic.m_req.m_trunked.m_state == proxy_http_svr_basic_trunked_length) { */
    /*         char * data; */
    /*         uint32_t size; */
    /*         if (net_endpoint_buf_by_str(endpoint, net_ep_buf_read, "\r\n", (void**)&data, &size) != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: search trunk len fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         if (data == NULL) { */
    /*             if(net_endpoint_buf_size(endpoint, net_ep_buf_read) > 30) { */
    /*                 CPE_ERROR( */
    /*                     http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: length linke too big! size=%d", */
    /*                     net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                     net_endpoint_buf_size(endpoint, net_ep_buf_read)); */
    /*                 return -1; */
    /*             } */
    /*             else { */
    /*                 return 0; */
    /*             } */
    /*         } */

    /*         char * endptr = NULL; */
    /*         http_ep->m_basic.m_req.m_trunked.m_length = (uint32_t)strtol(data, &endptr, 16); */
    /*         if (endptr == NULL || *endptr != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunk-length %s format error", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 data); */
    /*             return -1; */
    /*         } */

    /*         if (net_endpoint_protocol_debug(endpoint)) { */
    /*             CPE_INFO( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunk[%d].length = %s(%d)", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 http_ep->m_basic.m_req.m_trunked.m_count, */
    /*                 data, */
    /*                 http_ep->m_basic.m_req.m_trunked.m_length); */
    /*         } */

    /*         assert(size == (uint32_t)strlen(data) + 2); */
    /*         memcpy(data + (size - 2), "\r\n", 2); */
    /*         if (net_endpoint_buf_append(endpoint, net_ep_buf_forward, data, size) != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> write trunk length fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         net_endpoint_buf_consume(endpoint, net_ep_buf_read, size); */

    /*         http_ep->m_basic.m_req.m_trunked.m_state = */
    /*             http_ep->m_basic.m_req.m_trunked.m_length == 0 */
    /*             ? proxy_http_svr_basic_trunked_complete */
    /*             : proxy_http_svr_basic_trunked_content; */
    /*     } */
    /*     else if (http_ep->m_basic.m_req.m_trunked.m_state == proxy_http_svr_basic_trunked_content) { */
    /*         if (forward_sz > http_ep->m_basic.m_req.m_trunked.m_length) { */
    /*             forward_sz = http_ep->m_basic.m_req.m_trunked.m_length; */
    /*         } */

    /*         if (net_endpoint_protocol_debug(endpoint)) { */
    /*             CPE_INFO( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunk[%d]: %d data(left=%d)", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 http_ep->m_basic.m_req.m_trunked.m_count, */
    /*                 forward_sz, http_ep->m_basic.m_req.m_trunked.m_length - forward_sz); */
    /*         } */

    /*         if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, forward_sz) != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> forward trunk %d data(%d) fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 http_ep->m_basic.m_req.m_trunked.m_count, */
    /*                 forward_sz); */
    /*             return -1; */
    /*         } */

    /*         http_ep->m_basic.m_req.m_trunked.m_length -= forward_sz; */
    /*         if (http_ep->m_basic.m_req.m_trunked.m_length == 0) { */
    /*             http_ep->m_basic.m_req.m_trunked.m_state = proxy_http_svr_basic_trunked_content_complete; */
    /*         } */
    /*     } */
    /*     else if (http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_content_complete) { */
    /*         if (forward_sz < 2) return 0; */
            
    /*         char * data; */
    /*         net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_read, 2, (void**)&data); */
    /*         assert(data); */

    /*         if (data[0] != '\r' || data[1] != '\n') { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: read trunk rn fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, 2) != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: write trunk rn fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         http_ep->m_basic.m_req.m_trunked.m_state = proxy_http_svr_basic_trunked_length; */
    /*         http_ep->m_basic.m_req.m_trunked.m_count++; */
    /*     } */
    /*     else { */
    /*         assert(http_ep->m_basic.m_rsp.m_trunked.m_state == proxy_http_svr_basic_trunked_complete); */

    /*         if (forward_sz < 2) return 0; */
            
    /*         char * data; */
    /*         net_endpoint_buf_peak_with_size(endpoint, net_ep_buf_read, 2, (void**)&data); */
    /*         assert(data); */

    /*         if (data[0] != '\r' || data[1] != '\n') { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: read last data fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         if (net_endpoint_buf_append_from_self(endpoint, net_ep_buf_forward, net_ep_buf_read, 2) != 0) { */
    /*             CPE_ERROR( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: write last rn fail", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*             return -1; */
    /*         } */

    /*         if (net_endpoint_protocol_debug(endpoint)) { */
    /*             CPE_INFO( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: basic: ==> trunked: transfer completed, trunk-count=%d", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 http_ep->m_basic.m_req.m_trunked.m_count); */
    /*         } */

    /*         net_proxy_http_svr_endpoint_basic_req_set_state( */
    /*             http_protocol, http_ep, endpoint, */
    /*             http_ep->m_basic.m_connection == proxy_http_connection_keep_alive */
    /*             ? proxy_http_svr_basic_req_state_header */
    /*             : proxy_http_svr_basic_req_state_stop); */
    /*         return 0; */
    /*     } */
    /* } */
    
    return 0;
}

int net_proxy_http_svr_endpoint_basic_req_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    /*初始化ctx */
    /* struct net_proxy_http_svr_endpoint_basic_req_head_context ctx; */
    /* bzero(&ctx, sizeof(ctx)); */
    /* net_endpoint_writer_init(&ctx.m_output, endpoint, net_ep_buf_forward); */

    /* char * line = data; */
    /* char * sep = strstr(line, "\r\n"); */
    /* uint32_t line_num = 0; */
    /* for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) { */
    /*     *sep = 0; */
    /*     if (line_num == 0) { */
    /*         if (net_proxy_http_svr_endpoint_basic_req_parse_header_method(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR; */
    /*     } */
    /*     else { */
    /*         if (net_proxy_http_svr_endpoint_basic_req_parse_header_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR; */
    /*     } */
    /*     line_num++; */
    /* } */

    /* if (line[0]) { */
    /*     if (line_num == 0) { */
    /*         if (net_proxy_http_svr_endpoint_basic_req_parse_header_method(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR; */
    /*     } */
    /*     else { */
    /*         if (net_proxy_http_svr_endpoint_basic_req_parse_header_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) goto READ_HEAD_ERROR; */
    /*     } */
    /* } */

    /* if (http_ep->m_basic.m_connection == proxy_http_connection_unknown) { */
    /*     http_ep->m_basic.m_connection = */
    /*         http_ep->m_basic.m_version == proxy_http_version_1_0 */
    /*         ? proxy_http_connection_close */
    /*         : proxy_http_connection_keep_alive; */
    /* } */

    /* if (http_ep->m_basic.m_version == proxy_http_version_1_1 */
    /*     && !ctx.m_header_has_connection) */
    /* { */
    /*     if (net_endpoint_writer_append_str(&ctx.m_output, "Connection: ") != 0 */
    /*         || net_endpoint_writer_append_str(&ctx.m_output, http_ep->m_basic.m_connection == proxy_http_connection_close ? "Close" : "Keep-Alive") != 0 */
    /*         || net_endpoint_writer_append_str(&ctx.m_output, "\r\n") != 0) */
    /*     { */
    /*         goto READ_HEAD_ERROR; */
    /*     } */
    /* } */

    /* if (net_endpoint_writer_append_str(&ctx.m_output, "\r\n") != 0) goto READ_HEAD_ERROR; */
    /* if (net_endpoint_writer_commit(&ctx.m_output) != 0) goto READ_HEAD_ERROR; */

    /* if (net_endpoint_protocol_debug(endpoint)) { */
    /*     CPE_INFO( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: basic: ==> head(%d)", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*         ctx.m_output.m_totall_len); */
    /*     if (net_endpoint_protocol_debug(endpoint) >= 2) { */
    /*         net_proxy_http_svr_endpoint_dump_content_text(http_protocol, endpoint, net_ep_buf_forward, ctx.m_output.m_totall_len - 4); */
    /*     } */
    /* } */

    /* if (net_proxy_http_svr_endpoint_basic_forward_direct_remote(http_protocol, http_ep, endpoint, ctx.m_host) != 0) goto READ_HEAD_ERROR; */
    
    /* if (net_endpoint_forward(endpoint) != 0) { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: basic: forward head to other fail!", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*     goto READ_HEAD_ERROR; */
    /* } */

    /* if (ctx.m_host) net_address_free(ctx.m_host); */
    /* net_proxy_http_svr_endpoint_basic_req_set_state(http_protocol, http_ep, endpoint, proxy_http_svr_basic_req_state_content); */
    
    return 0;

READ_HEAD_ERROR:
    /* net_endpoint_writer_cancel(&ctx.m_output); */
    /* if (ctx.m_host) net_address_free(ctx.m_host); */
    return -1;
}

static int net_proxy_http_svr_endpoint_basic_req_parse_header_method(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_req_head_context * ctx, char * line)
{
    char * sep1 = strchr(line, ' ');
    char * sep2 = sep1 ? strchr(sep1 + 1, ' ') : NULL;
    if (sep1 == NULL || sep2 == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: parse head method: first line %s format error",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            line);
        return -1;
    }

    char * method = line;
    *sep1 = 0;
    ctx->m_method = method;

    const char * origin_target = sep1 + 1;
    *sep2 = 0;

    const char * version = sep2 + 1;

    if (strcasecmp(version, "HTTP/1.0") == 0) {
        http_ep->m_basic.m_version = proxy_http_version_1_0;
    }
    else if (strcasecmp(version, "HTTP/1.1") == 0) {
        http_ep->m_basic.m_version = proxy_http_version_1_1;
    }
    else {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: not support mehtod %s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            version);
        return -1;
    }
    
    const char * target = origin_target;
    uint8_t remove_host_name = 0;
    if (cpe_str_start_with(target, "http://")) {
        target += 7;
        remove_host_name = 1;
    }
    else if (cpe_str_start_with(target, "https://")) {
        target += 8;
        remove_host_name = 1;
    }

    if (remove_host_name) {
        sep1 = strchr(target, '/');
        if (sep1) {
            target = sep1;
        }
        else {
            target = "/";
        }
    }

    if (net_endpoint_writer_append_str(&ctx->m_output, method) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, " ") != 0
        || net_endpoint_writer_append_str(&ctx->m_output, target) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, " ") != 0
        || net_endpoint_writer_append_str(&ctx->m_output, version) != 0
        || net_endpoint_writer_append_str(&ctx->m_output, "\r\n") != 0)
        return -1;

    return 0;
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
    http_ep->m_basic.m_req.m_content_text = net_proxy_http_svr_endpoint_is_mine_text(value);
}

static int net_proxy_http_svr_endpoint_basic_forward_direct_remote(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, net_address_t host)
{
    /* /\*设定remote *\/ */
    /* if (host == NULL) { */
    /*     CPE_ERROR( */
    /*         http_protocol->m_em, "http-proxy-svr: %s: basic: no host in header!", */
    /*         net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint)); */
    /*     return -1; */
    /* } */
    
    /* char str_address[128]; */
    /* cpe_str_dup( */
    /*     str_address, sizeof(str_address), */
    /*     net_address_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), host)); */

    /* uint8_t need_connect = 0; */
    /* net_endpoint_t other = net_endpoint_other(endpoint); */
    /* if (other == NULL) { */
    /*     need_connect = 1; */
    /*     net_endpoint_set_remote_address(endpoint, host); */

    /*     if (net_endpoint_protocol_debug(endpoint)) { */
    /*         CPE_INFO( */
    /*             http_protocol->m_em, "http-proxy-svr: %s: request first connect to %s!", */
    /*             net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*             str_address); */
    /*     } */
    /* } */
    /* else { */
    /*     net_address_t cur_remote_address = net_endpoint_remote_address(endpoint); */
    /*     if (net_address_cmp(cur_remote_address, host) != 0) { */
    /*         need_connect = 1; */
    /*         net_endpoint_set_remote_address(endpoint, host); */

    /*         if (net_endpoint_protocol_debug(endpoint)) { */
    /*             CPE_INFO( */
    /*                 http_protocol->m_em, "http-proxy-svr: %s: request re connect to %s!", */
    /*                 net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint), */
    /*                 str_address); */
    /*         } */
    /*     } */
    /* } */

    /* if (need_connect */
    /*     && http_ep->m_on_connect_fun */
    /*     && http_ep->m_on_connect_fun(http_ep->m_on_connect_ctx, endpoint, host) != 0) */
    /* { */
    /*     return -1; */
    /* } */

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_req_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_req_head_context * ctx, char * line)
{
    char * sep = strchr(line, ':');
    if (sep == NULL) return 0;

    char * name = line;
    char * value = cpe_str_trim_head(sep + 1);

    *cpe_str_trim_tail(sep, line) = 0;

    uint8_t keep_line = 1;
    if (strcasecmp(name, "Proxy-Authenticate") == 0) {
        keep_line = 0;
    }
    else if (strcasecmp(name, "Proxy-Authorization") == 0) {
        keep_line = 0;
    }
    else if (strcasecmp(name, "Proxy-Connection") == 0) {
        keep_line = 0;
        cpe_str_list_for_each(value, ';', process_connection, http_ep);
    }
    else if (strcasecmp(name, "Connection") == 0) {
        ctx->m_header_has_connection = 1;
    }
    else if (strcasecmp(name, "Content-Type") == 0) {
        cpe_str_list_for_each(value, ';', process_context_type, http_ep);
    }
    else if (strcasecmp(name, "Content-Encoding") == 0) {
        http_ep->m_basic.m_req.m_content_coded = 1;
    }
    else if (strcasecmp(name, "Host") == 0) {
        net_address_t address = net_address_create_auto(net_proxy_http_svr_protocol_schedule(http_protocol), value);
        if (address == NULL) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: Host %s format error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            return -1;
        }
        if (net_address_port(address) == 0) net_address_set_port(address, 80);

        if (ctx->m_host) {
            if (net_address_cmp(ctx->m_host, address) != 0) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: Host mismatch",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
                net_address_free(address);
                return -1;
            }
            else {
                net_address_free(address);
            }
        }
        else {
            ctx->m_host = address;
        }
    }
    else if (strcasecmp(name, "Content-Length") == 0) {
        http_ep->m_basic.m_req.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        
        char * endptr = NULL;
        http_ep->m_basic.m_req.m_content.m_length = (uint32_t)strtol(value, &endptr, 10);
        if (endptr == NULL || *endptr != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: ==> Content-Length %s format error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            return -1;
        }
    }
    else if (strcasecmp(name, "Transfer-Encoding") == 0) {
        if (strcasecmp(value, "chunked") == 0) {
            http_ep->m_basic.m_req.m_trans_encoding = proxy_http_svr_basic_trans_encoding_trunked;
            http_ep->m_basic.m_req.m_trunked.m_state = proxy_http_svr_basic_trunked_length;
            http_ep->m_basic.m_req.m_trunked.m_count = 0;
            http_ep->m_basic.m_req.m_trunked.m_length = 0;
        }
        else {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: ==> Transfer-Encoding %s not support",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            return -1;
        }
    }
    
    if (keep_line) {
        if (net_endpoint_writer_append_str(&ctx->m_output, name) != 0
            || net_endpoint_writer_append_str(&ctx->m_output, ": ") != 0
            || net_endpoint_writer_append_str(&ctx->m_output, value) != 0
            || net_endpoint_writer_append_str(&ctx->m_output, "\r\n") != 0)
            return -1;
    }
    
    return 0;
}

static void net_proxy_http_svr_endpoint_basic_req_set_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_req_state_t req_state)
{
    if (http_ep->m_basic.m_req.m_state == req_state) return;

    if (net_endpoint_protocol_debug(endpoint) >= 2) {
        if (req_state == proxy_http_svr_basic_req_state_content) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: req-state %s ==> %s, content-length=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_req_state_str(http_ep->m_basic.m_req.m_state),
                proxy_http_svr_basic_req_state_str(req_state),
                http_ep->m_basic.m_req.m_content.m_length);
        }
        else {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: basic: req-state %s ==> %s",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_req_state_str(http_ep->m_basic.m_req.m_state),
                proxy_http_svr_basic_req_state_str(req_state));
        }
    }

    http_ep->m_basic.m_req.m_state = req_state;

    if (req_state == proxy_http_svr_basic_req_state_header) {
        http_ep->m_basic.m_req.m_trans_encoding = proxy_http_svr_basic_trans_encoding_none;
        http_ep->m_basic.m_req.m_content_text = 0;
        http_ep->m_basic.m_req.m_content_coded = 0;
        http_ep->m_basic.m_req.m_content.m_length = 0;
    }
}

const char * proxy_http_svr_basic_req_state_str(proxy_http_svr_basic_req_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_req_state_header:
        return "req-header";
    case proxy_http_svr_basic_req_state_content:
        return "req-content";
    case proxy_http_svr_basic_req_state_stop:
        return "req-stop";
    }
}

