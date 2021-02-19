#ifndef NET_SMUX_DGRAM_H_INCLEDED
#define NET_SMUX_DGRAM_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_smux_types.h"

NET_BEGIN_DECL

net_smux_dgram_t
net_smux_dgram_create(
    net_smux_protocol_t protocol,
    net_smux_runing_mode_t runing_mode,
    net_driver_t driver,
    net_address_t local_address,
    net_smux_config_t config);

void net_smux_dgram_free(net_smux_dgram_t dgram);

net_dgram_t net_smux_dgram_dgram(net_smux_dgram_t dgram);

net_smux_session_t
net_smux_dgram_find_session(net_smux_dgram_t dgram, net_address_t remote_address);

net_smux_session_t
net_smux_dgram_open_session(net_smux_dgram_t dgram, net_address_t remote_address);

net_smux_session_t
net_smux_dgram_check_open_session(net_smux_dgram_t dgram, net_address_t remote_address);

void net_smux_dgram_print(write_stream_t ws, net_smux_dgram_t dgram);
const char * net_smux_dgram_dump(mem_buffer_t buffer, net_smux_dgram_t dgram);

NET_END_DECL

#endif
