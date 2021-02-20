#include "cpe/pal/pal_platform.h"
#include "cpe/utils/stream_buffer.h"
#include "net_xkcp_utils.h"

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

