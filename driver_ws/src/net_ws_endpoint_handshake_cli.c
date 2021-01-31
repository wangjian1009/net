#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/random.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_ws_endpoint_i.h"
#include "net_ws_stream_endpoint_i.h"
#include "net_ws_utils.h"

#define net_ws_endpoint_handshake_cli_field_is_set(__field) \
    (endpoint->m_state_data.m_cli.m_handshake.m_received_fields & (1u << __field))
#define net_ws_endpoint_handshake_cli_field_set(__field) \
    (endpoint->m_state_data.m_cli.m_handshake.m_received_fields |= (1u << __field))

const char * net_ws_endpoint_handshake_cli_field_str(net_ws_endpoint_handshake_cli_field_t field);

int net_ws_endpoint_input_handshake_cli_first_line(
    net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint, char * line, uint32_t line_sz)
{
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    char * sep = strchr(line, ' ');
    if (sep == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: first line sep 1 not found: %s!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            line);
        return -1;
    }
    char * http_protocol = line; * cpe_str_trim_tail(sep, http_protocol) = 0;
    line = cpe_str_trim_head(sep + 1);

    sep = strchr(line, ' ');
    if (sep == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: first line sep 2 not found: %s!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            line);
        return -1;
    }
    char * code = line; * cpe_str_trim_tail(sep, code) = 0;

    char * code_msg = cpe_str_trim_head(sep + 1);
    
    if (strcasecmp(http_protocol, "HTTP/1.1") != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: http protocol %s not suport!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            http_protocol);
        return -1;
    }

    return 0;
}

int net_ws_endpoint_input_handshake_cli_header_line(
    net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint, char * line, uint32_t line_sz)
{
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    char * sep = strchr(line, ':');
    if (sep == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: line no sep: %s!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            line);
        return -1;
    }

    char * name = line; * cpe_str_trim_tail(sep, line) = 0;
    char * value = cpe_str_trim_head(sep + 1);

    if (strcasecmp(name, "Upgrade") == 0) {
        if (strcasecmp(value, "websocket") != 0) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: handshake: upgrade type %s not support!",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                value);
            return -1;
        }
        net_ws_endpoint_handshake_cli_field_set(net_ws_endpoint_handshake_cli_field_upgrade);
    }
    else if (strcasecmp(name, "Connection") == 0) {
        if (strcasecmp(value, "Upgrade") != 0) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: handshake: connection value %s not support!",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                value);
            return -1;
        }
        net_ws_endpoint_handshake_cli_field_set(net_ws_endpoint_handshake_cli_field_connection);
    }
    else if (strcasecmp(name, "Sec-WebSocket-Accept") == 0) {
        //TODO: 验证accept正确性
        net_ws_endpoint_handshake_cli_field_set(net_ws_endpoint_handshake_cli_field_accept);
    }
    
    return 0;
}

int net_ws_endpoint_input_handshake_cli_last_line(
    net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint)
{
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    /*检查必须的字段 */
    net_ws_endpoint_handshake_cli_field_t check_fields[] = {
        net_ws_endpoint_handshake_cli_field_upgrade,
        net_ws_endpoint_handshake_cli_field_connection,
        net_ws_endpoint_handshake_cli_field_accept
    };
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(check_fields); ++i) {
        if (!net_ws_endpoint_handshake_cli_field_is_set(i)) {
            CPE_ERROR(
                protocol->m_em, "net: ws: %s: handshake: field %s not set!",
                net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                net_ws_endpoint_handshake_cli_field_str((net_ws_endpoint_handshake_cli_field_t)i));
            return -1;
        }
    }

    return 0;
}

int net_ws_endpoint_input_handshake_cli(net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    while(endpoint->m_state == net_ws_endpoint_state_handshake) {
        uint32_t line_size = 0;
        char * line = NULL;

        /*读取错误（输入数据流有错误） */
        if (net_endpoint_buf_by_str(base_endpoint, net_ep_buf_read, "\r\n", (void**)&line, &line_size)) return -1;

        /*数据不足够 */
        if (line_size == 0) {
            uint32_t total_size =
                endpoint->m_state_data.m_cli.m_handshake.m_readed_size
                + net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
            if (total_size >= 8192) {
                CPE_ERROR(
                    protocol->m_em, "net: ws: %s: handshake << %d, Too large http header",
                    net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
                    total_size);
                return -1;
            }

            return 0;
        }

        assert(line_size >= 2);
        if (line[0] == 0) {
            if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: handshake completed",
                    net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
            }

            if (net_ws_endpoint_input_handshake_cli_last_line(base_endpoint, endpoint) != 0) return -1;
            net_ws_endpoint_set_state(endpoint, net_ws_endpoint_state_streaming);
        }
        else {
            if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: handshake << %s",
                    net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint), line);
            }

            switch (endpoint->m_state_data.m_cli.m_handshake.m_state) {
            case net_ws_endpoint_handshake_cli_first_line:
                if (net_ws_endpoint_input_handshake_cli_first_line(
                        base_endpoint, endpoint, line, line_size - 2) != 0) return -1;
                endpoint->m_state_data.m_cli.m_handshake.m_state = net_ws_endpoint_handshake_cli_header_line;
                break;
            case net_ws_endpoint_handshake_cli_header_line:
                if (net_ws_endpoint_input_handshake_cli_header_line(
                        base_endpoint, endpoint, line, line_size - 2) != 0) return -1;
            }
        }

        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, line_size);
        endpoint->m_state_data.m_cli.m_handshake.m_readed_size += line_size;
    }

    return 0;
}

int net_ws_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_endpoint_t endpoint) {
    net_ws_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_path == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no path!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no remote address!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    uint32_t buf_capacity = 4096;
    char * buf = net_endpoint_buf_alloc_at_least(base_endpoint, &buf_capacity);
    if (buf == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: alloc buf fail!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, buf_capacity);
    
    stream_printf((write_stream_t)&ws, "GET %s HTTP/1.1\r\n", endpoint->m_path);

    stream_printf((write_stream_t)&ws, "Host: ");
    net_address_print((write_stream_t)&ws, target_addr);
    stream_printf((write_stream_t)&ws, "\r\n");
    
    stream_printf((write_stream_t)&ws, "Upgrade: websocket\r\n");
    stream_printf((write_stream_t)&ws, "Connection: Upgrade\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Key: ");
    cpe_rand_ctx_fill(
        cpe_rand_ctx_dft(),
        &endpoint->m_state_data.m_cli.m_handshake.m_key,
        sizeof(endpoint->m_state_data.m_cli.m_handshake.m_key));
    cpe_base64_encode_from_buf(
        (write_stream_t)&ws,
        endpoint->m_state_data.m_cli.m_handshake.m_key,
        sizeof(endpoint->m_state_data.m_cli.m_handshake.m_key));
    stream_printf((write_stream_t)&ws, "\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Version: 13\r\n");
    stream_printf((write_stream_t)&ws, "\r\n");

    uint32_t size = ws.m_pos;

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data\n%.*s",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            size, size, buf);
    }
    
    if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_write, size) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data failed!",
            net_endpoint_dump(net_ws_protocol_tmp_buffer(protocol), base_endpoint),
            size);
        return -1;
    }

    return 0;
}

const char * net_ws_endpoint_handshake_cli_field_str(net_ws_endpoint_handshake_cli_field_t field) {
    switch(field) {
    case net_ws_endpoint_handshake_cli_field_upgrade:
        return "upgrade";
    case net_ws_endpoint_handshake_cli_field_connection:
        return "connection";
    case net_ws_endpoint_handshake_cli_field_accept:
        return "accept";
    }
}
