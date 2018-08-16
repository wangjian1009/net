#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_proxy_http_svr_endpoint_i.h"

const char * proxy_http_svr_basic_read_state_str(proxy_http_svr_basic_read_state_t state);
const char * proxy_http_svr_basic_write_state_str(proxy_http_svr_basic_write_state_t state);

static void net_proxy_http_svr_endpoint_basic_set_read_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_read_state_t read_state);

struct net_proxy_http_svr_endpoint_basic_head_context {
    net_endpoint_t m_other;
    uint8_t m_keep_alive;
    uint32_t m_context_length;
    char * m_output;
    uint32_t m_output_size;
    uint32_t m_output_capacity;
};

static int net_proxy_http_svr_endpoint_basic_append_header(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_head_context * ctx, const char * name, const char * value);

static int net_proxy_http_svr_endpoint_basic_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_head_context * ctx, char * line);

int net_proxy_http_svr_endpoint_basic_read_head(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint, char * data)
{
    /*初始化ctx */
    struct net_proxy_http_svr_endpoint_basic_head_context ctx;
    bzero(&ctx, sizeof(ctx));
    ctx.m_other = net_endpoint_other(endpoint);
    ctx.m_output_capacity = (uint32_t)strlen(data);

    mem_buffer_clear_data(&http_protocol->m_data_buffer);
    ctx.m_output = mem_buffer_alloc(&http_protocol->m_data_buffer, ctx.m_output_capacity);
    
    char * line = data;
    char * sep = strstr(line, "\r\n");
    for(; sep; line = sep + 2, sep = strstr(line, "\r\n")) {
        *sep = 0;
        if (net_proxy_http_svr_endpoint_basic_parse_header_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) return -1;
        *sep = '\r';
    }

    if (line[0]) {
        if (net_proxy_http_svr_endpoint_basic_parse_header_line(http_protocol, http_ep, endpoint, &ctx, line) != 0) return -1;
    }

    if (ctx.m_other == NULL) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: after read head, no linked other endpoint, error!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }

    if (ctx.m_context_length == 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: after read head, no content length, error!",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint));
        return -1;
    }
    
    if (net_endpoint_fbuf_append(ctx.m_other, ctx.m_output, ctx.m_output_capacity) != 0) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: forward head to other fail!\n%s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            ctx.m_output);
        return -1;
    }

    http_ep->m_basic.m_read_context_length = ctx.m_context_length;
    net_proxy_http_svr_endpoint_basic_set_read_state(http_protocol, http_ep, endpoint, proxy_http_svr_basic_read_state_reading_content);

    if (http_ep->m_debug) {
        CPE_INFO(
            http_protocol->m_em, "http-proxy-svr: %s: ==> %s",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            ctx.m_output);
    }

    return 0;
}

static int net_proxy_http_svr_endpoint_basic_append_header(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_head_context * ctx, const char * name, const char * value)
{
    assert(ctx->m_output_size + 1u < ctx->m_output_capacity);

    uint32_t left_capacity = ctx->m_output_capacity - ctx->m_output_size - 1;
    int n = snprintf(ctx->m_output + ctx->m_output_size, left_capacity, "%s: %s\r\n", name, value);
    if (n == left_capacity) {
        CPE_ERROR(
            http_protocol->m_em, "http-proxy-svr: %s: append header %s: overflow, capacity=%d, left-capacity=%d",
            net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
            name, ctx->m_output_capacity, left_capacity);
        return -1;
    }

    ctx->m_output_size += (uint32_t)n;
    
    return 0;
}

static int net_proxy_http_svr_endpoint_basic_parse_header_line(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    struct net_proxy_http_svr_endpoint_basic_head_context * ctx,
    char * line)
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
        http_ep->m_keep_alive = 1;
        keep_line = 0;
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

        if (ctx->m_other == NULL) {
            char str_address[128];
            cpe_str_dup(
                str_address, sizeof(str_address),
                net_address_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), address));

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

            ctx->m_other = net_endpoint_other(endpoint);
            if (ctx->m_other == NULL) {
                CPE_ERROR(
                    http_protocol->m_em, "http-proxy-svr: %s: request connect to %s, link fail!(no other endpoint)",
                    net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                    str_address);
            }
        }
    }
    else if (strcasecmp(name, "Content-Length") == 0) {
        char * endptr = NULL;
        ctx->m_context_length = (uint32_t)strtol(value, &endptr, 10);
        if (endptr == NULL || *endptr != 0) {
            CPE_ERROR(
                http_protocol->m_em, "http-proxy-svr: %s: Content-Length %s format error",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                value);
            return -1;
        }
    }
    else if (strcasecmp(name, "Connection") == 0) {
        ctx->m_keep_alive = strcasecmp(name, "keep-alive") == 0 ? 1 : 0;
    }
    
    if (keep_line) {
        if (net_proxy_http_svr_endpoint_basic_append_header(http_protocol, http_ep, endpoint, ctx, name, value) != 0) return -1;
    }
    
    return 0;
}

static void net_proxy_http_svr_endpoint_basic_set_read_state(
    net_proxy_http_svr_protocol_t http_protocol, net_proxy_http_svr_endpoint_t http_ep, net_endpoint_t endpoint,
    proxy_http_svr_basic_read_state_t read_state)
{
    if (http_ep->m_basic.m_read_state == read_state) return;

    if (http_ep->m_debug) {
        if (read_state == proxy_http_svr_basic_read_state_reading_content) {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: read-state %s ==> %s, content-length=%d",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_read_state_str(http_ep->m_basic.m_read_state),
                proxy_http_svr_basic_read_state_str(read_state),
                http_ep->m_basic.m_read_context_length);
        }
        else {
            CPE_INFO(
                http_protocol->m_em, "http-proxy-svr: %s: read-state %s ==> %s",
                net_endpoint_dump(net_proxy_http_svr_protocol_tmp_buffer(http_protocol), endpoint),
                proxy_http_svr_basic_read_state_str(http_ep->m_basic.m_read_state),
                proxy_http_svr_basic_read_state_str(read_state));
        }
    }

    http_ep->m_basic.m_read_state = read_state;
}

const char * proxy_http_svr_basic_read_state_str(proxy_http_svr_basic_read_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_read_state_reading_header:
        return "r-reading-header";
    case proxy_http_svr_basic_read_state_reading_content:
        return "r-reading-content";
    }
}

const char * proxy_http_svr_basic_write_state_str(proxy_http_svr_basic_write_state_t state) {
    switch(state) {
    case proxy_http_svr_basic_write_state_invalid:
        return "w-invalid";
    case proxy_http_svr_basic_write_state_sending_content_response:
        return "w-sending-content-response";
    case proxy_http_svr_basic_write_state_forwarding:
        return "w-forwarding";
    case proxy_http_svr_basic_write_state_stopped:
        return "w-stoped";
    }
}
