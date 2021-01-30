#include <assert.h>
#include "cpe/utils/error.h"
#include "cpe/utils/hex_utils.h"
#include "cpe/utils/base64.h"
#include "cpe/utils/random.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/string_utils.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_ws_cli_endpoint_i.h"
#include "net_ws_cli_stream_endpoint_i.h"
#include "net_ws_utils.h"

int net_ws_cli_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_cli_endpoint_t endpoint) {
    net_ws_cli_protocol_t protocol = net_protocol_data(net_endpoint_protocol(base_endpoint));

    if (endpoint->m_path == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no path!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    net_address_t target_addr = net_endpoint_remote_address(base_endpoint);
    if (target_addr == NULL) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send: no remote address!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint));
        return -1;
    }

    uint32_t buf_capacity = 4096;
    char * buf = net_endpoint_buf_alloc_at_least(base_endpoint, &buf_capacity);
    struct write_stream_mem ws = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, buf_capacity);
    
    stream_printf((write_stream_t)&ws, "GET %s HTTP/1.1\r\n", endpoint->m_path);

    stream_printf((write_stream_t)&ws, "Host: ");
    net_address_print((write_stream_t)&ws, target_addr);
    stream_printf((write_stream_t)&ws, "\r\n");
    
    stream_printf((write_stream_t)&ws, "Upgrade: websocket\r\n");
    stream_printf((write_stream_t)&ws, "Connection: Upgrade\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Key: ");
    char client_key[16];
    cpe_rand_ctx_fill(cpe_rand_ctx_dft(), client_key, sizeof(client_key));
    cpe_base64_encode_from_buf((write_stream_t)&ws, client_key, sizeof(client_key));
    stream_printf((write_stream_t)&ws, "\r\n");

    stream_printf((write_stream_t)&ws, "Sec-WebSocket-Version: 13\r\n");
    stream_printf((write_stream_t)&ws, "\r\n");

    uint32_t size = ws.m_pos;

    if (net_endpoint_protocol_debug(base_endpoint) >= 2) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data\n%.*s",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
            size, size, buf);
    }
    
    if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_write, size) != 0) {
        CPE_ERROR(
            protocol->m_em, "net: ws: %s: handshake: send %d data failed!",
            net_endpoint_dump(net_ws_cli_protocol_tmp_buffer(protocol), base_endpoint),
            size);
        return -1;
    }

    return 0;
}
