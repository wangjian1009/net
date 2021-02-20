#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
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

void net_xkcp_print_frame(write_stream_t ws, const void * data, uint32_t data_len) {
    if (data_len < 24) {
        stream_printf(ws, "len %d too small", data_len);
        return;
    }

    uint32_t conv;
    
}

const char * net_xkcp_dump_frame(mem_buffer_t buffer, const void * data, uint32_t data_len) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_xkcp_print_frame((write_stream_t)&ws, data, data_len);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

