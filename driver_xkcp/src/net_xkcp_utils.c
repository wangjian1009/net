#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_xkcp_config.h"
#include "net_xkcp_utils.h"

void net_xkcp_apply_config(ikcpcb * kcp, net_xkcp_config_t config) {
    ikcp_setmtu(kcp, config->m_mtu);
	ikcp_wndsize(kcp, config->m_send_wnd, config->m_recv_wnd);
    
    switch(config->m_mode) {
    case net_xkcp_mode_normal:
        ikcp_nodelay(kcp, 0, 40, 2, 1);
        break;
    case net_xkcp_mode_fast:
        ikcp_nodelay(kcp, 0, 30, 2, 1);
        break;
    case net_xkcp_mode_fast2:
        ikcp_nodelay(kcp, 1, 20, 2, 1);
        break;
    case net_xkcp_mode_fast3:
        ikcp_nodelay(kcp, 1, 10, 2, 1);
        break;
    }
}

void net_xkcp_print_frame(write_stream_t ws, const void * i_data, uint32_t data_len) {
    if (data_len < 24) {
        stream_printf(ws, "len %d too small", data_len);
        return;
    }

    uint8_t const * data = i_data;
    
    uint32_t conv;
    CPE_COPY_LTOH32(&conv, data); data += 4;
    stream_printf(ws, "conv=%d: ", conv);

    uint8_t cmd = *data; data++;
    stream_printf(ws, "cmd=");
    switch(cmd) {
    case 81:
        stream_printf(ws, "push");
        break;
    case 82:
        stream_printf(ws, "ack");
        break;
    case 83:
        stream_printf(ws, "wask");
        break;
    case 84:
        stream_printf(ws, "wins");
        break;
    default:
        stream_printf(ws, "%d", cmd);
        break;
    }

    uint8_t frg = *data; data++;
    stream_printf(ws, ", frg=%d", frg);

    uint16_t wnd;
    CPE_COPY_LTOH16(&wnd, data); data += 2;
    stream_printf(ws, ", wnd=%u", wnd);

    uint32_t ts;
    CPE_COPY_LTOH32(&ts, data); data += 4;
    stream_printf(ws, ", ts=%u", ts);
    
    uint32_t sn;
    CPE_COPY_LTOH32(&sn, data); data += 4;
    stream_printf(ws, ", sn=%u", sn);

    uint32_t uma;
    CPE_COPY_LTOH32(&uma, data); data += 4;
    stream_printf(ws, ", uma=%u", uma);

    uint32_t len;
    CPE_COPY_LTOH32(&len, data); data += 4;
    stream_printf(ws, ", len=%u", len);
}

void net_xkcp_print_address_pair(write_stream_t ws, net_address_t local, net_address_t remote, uint8_t is_out) {
    if (local) {
        net_address_print(ws, local);
    }
    else {
        stream_printf(ws, "N/A");
    }

    stream_printf(ws, "--");

    if (remote) {
        net_address_print(ws, remote);
    }
    else {
        stream_printf(ws, "N/A");
    }

    if (is_out) {
        stream_printf(ws, ": ==>");
    }
    else {
        stream_printf(ws, ": <==");
    }
}

const char * net_xkcp_dump_frame(
    mem_buffer_t buffer,
    net_address_t local, net_address_t remote, uint8_t is_out,
    const void * data, uint32_t data_len)
{
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_xkcp_print_address_pair((write_stream_t)&ws, local, remote, is_out);
    stream_printf((write_stream_t)&ws, " ");
    net_xkcp_print_frame((write_stream_t)&ws, data, data_len);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

const char * net_xkcp_dump_address_pair(
    mem_buffer_t buffer,
    net_address_t local, net_address_t remote, uint8_t is_out)
{
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);
    net_xkcp_print_address_pair((write_stream_t)&ws, local, remote, is_out);
    stream_putc((write_stream_t)&ws, 0);
    return mem_buffer_make_continuous(buffer, 0);
}
