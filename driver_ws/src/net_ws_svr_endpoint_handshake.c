#include <assert.h>
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_ws_svr_endpoint_i.h"

int net_ws_svr_endpoint_input_handshake_send_error_response(
    net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint)
{
    return 0;
}

int net_ws_svr_endpoint_input_handshake_send_success_response(
    net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint)
{
        /* resheaders_("HTTP/1.1 101 Switching Protocols\r\n" */
        /*             "Upgrade: websocket\r\n" */
        /*             "Connection: Upgrade\r\n" */
        /*             "Sec-WebSocket-Accept: " + */
        /*             accept_key + */
        /*             "\r\n" */
        /*             "\r\n"), */
    return 0;
}

int net_ws_svr_endpoint_input_handshake_first_line(
    net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint, char * line, uint32_t line_sz)
{
    return 0;
}

int net_ws_svr_endpoint_input_handshake_header_line(
    net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint, char * line, uint32_t line_sz)
{
    return 0;
}

int net_ws_svr_endpoint_input_handshake_last_line(
    net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint)
{
    return 0;
}

int net_ws_svr_endpoint_input_handshake(net_endpoint_t base_endpoint, net_ws_svr_endpoint_t endpoint) {
    net_ws_svr_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    while(endpoint->m_state == net_ws_svr_endpoint_state_handshake) {
        uint32_t line_size = 0;
        char * line = NULL;

        /*读取错误（输入数据流有错误） */
        if (net_endpoint_buf_by_str(base_endpoint, net_ep_buf_read, "\r\n", (void**)&line, &line_size)) return -1;

        /*数据不足够 */
        if (line_size == 0) {
            uint32_t total_size = endpoint->m_handshake.m_readed_size + net_endpoint_buf_size(base_endpoint, net_ep_buf_read);
            if (total_size >= 8192) {
                CPE_ERROR(
                    protocol->m_em, "net: ws: %s: handshake << %d, Too large http header",
                    net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint),
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
                    net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint));
            }

            if (net_ws_svr_endpoint_input_handshake_last_line(base_endpoint, endpoint) != 0) return -1;
            endpoint->m_state = net_ws_svr_endpoint_state_streaming;
        }
        else {
            if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
                CPE_INFO(
                    protocol->m_em, "net: ws: %s: handshake << %s",
                    net_endpoint_dump(net_ws_svr_protocol_tmp_buffer(protocol), base_endpoint), line);
            }

            switch (endpoint->m_handshake.m_state) {
            case net_ws_svr_endpoint_handshake_first_line:
                if (net_ws_svr_endpoint_input_handshake_first_line(
                        base_endpoint, endpoint, line, line_size - 2) != 0) return -1;
                endpoint->m_handshake.m_state = net_ws_svr_endpoint_handshake_header_line;
                break;
            case net_ws_svr_endpoint_handshake_header_line:
                if (net_ws_svr_endpoint_input_handshake_header_line(
                        base_endpoint, endpoint, line, line_size - 2) != 0) return -1;
            }
        }

        net_endpoint_buf_consume(base_endpoint, net_ep_buf_read, line_size);
        endpoint->m_handshake.m_readed_size += line_size;
    }

    return 0;
}
