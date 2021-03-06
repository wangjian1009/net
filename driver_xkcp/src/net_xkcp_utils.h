#ifndef NET_XKCP_UTILS_I_H_INCLEDED
#define NET_XKCP_UTILS_I_H_INCLEDED
#include "net_xkcp_driver_i.h"

void net_xkcp_print_frame(write_stream_t ws, const void * data, uint32_t data_len);

const char * net_xkcp_dump_frame(
    mem_buffer_t buffer, const void * data, uint32_t data_len);

const char * net_xkcp_dump_address_pair(
    mem_buffer_t buffer,
    net_address_t local, net_address_t remote, uint8_t is_out);

const char * net_xkcp_dump_frame_with_address(
    mem_buffer_t buffer,
    net_address_t local, net_address_t remote, uint8_t is_out,
    const void * data, uint32_t data_len);

#endif
