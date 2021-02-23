#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "net_address.h"
#include "net_xkcp_config_i.h"

void net_xkcp_config_init_default(net_xkcp_config_t config) {
    config->m_mode = net_xkcp_mode_normal;
	config->m_mtu = 1350;
	config->m_send_wnd = 512;
	config->m_recv_wnd = 512;
}

uint8_t net_xkcp_config_validate(net_xkcp_config_t config, error_monitor_t em) {
    return 1;
}

const char * net_xkcp_mode_str(net_xkcp_mode_t mode) {
    switch(mode) {
    case net_xkcp_mode_normal:
        return "normal";
    case net_xkcp_mode_fast:
        return "fast";
    case net_xkcp_mode_fast2:
        return "fast2";
    case net_xkcp_mode_fast3:
        return "fast3";
    }
}

int net_xkcp_mode_from_str(net_xkcp_mode_t * mode, const char * str_mode) {
    if (strcmp(str_mode, "normal") == 0) {
        *mode = net_xkcp_mode_normal;
    }
    else if (strcmp(str_mode, "fast") == 0) {
        *mode = net_xkcp_mode_fast;
    }
    else if (strcmp(str_mode, "fast2") == 0) {
        *mode = net_xkcp_mode_fast2;
    }
    else if (strcmp(str_mode, "fast3") == 0) {
        *mode = net_xkcp_mode_fast3;
    }
    else {
        return -1;
    }
    return 0;
}

void net_xkcp_config_print(write_stream_t ws, net_xkcp_config_t config) {
    stream_printf(
        ws, "mode=%s, mut=d, send-wnd=%d, recv-wnd=%d",
        net_xkcp_mode_str(config->m_mode),
        config->m_mtu,
        config->m_send_wnd,
        config->m_recv_wnd);
}

const char * net_xkcp_config_dump(mem_buffer_t buffer, net_xkcp_config_t config) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_xkcp_config_print((write_stream_t)&ws, config);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}
