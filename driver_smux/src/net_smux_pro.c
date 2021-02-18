#include "cpe/utils/stream_buffer.h"
#include "net_smux_pro.h"

void net_smux_print_frame(write_stream_t ws, struct net_smux_head const * frame) {
    uint16_t len;
    CPE_COPY_NTOH16(&len, &frame->m_len);

    uint32_t sid;
    CPE_COPY_NTOH32(&sid, &frame->m_sid);
    
    switch(frame->m_cmd) {
    case net_smux_cmd_syn:
        stream_printf(ws, "syn %d(len=%d)", sid, len);
        break;
    case net_smux_cmd_fin:
        stream_printf(ws, "fin %d(len=%d)", sid, len);
        break;
    case net_smux_cmd_psh:
        stream_printf(ws, "psh");
        break;
    case net_smux_cmd_nop:
        stream_printf(ws, "nop");
        break;
    case net_smux_cmd_udp:
        stream_printf(ws, "udp");
        break;
    }
}

const char * net_smux_dump_frame(mem_buffer_t buffer, struct net_smux_head const * frame) {
    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    net_smux_print_frame((write_stream_t)&ws, frame);
    stream_putc((write_stream_t)&ws, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

