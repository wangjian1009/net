#ifndef NET_SMUX_SESSION_H_INCLEDED
#define NET_SMUX_SESSION_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_session_underline_type {
    net_smux_session_underline_udp,
    net_smux_session_underline_tcp,
};

net_smux_session_runing_mode_t net_smux_session_runing_mode(net_smux_session_t session);
net_smux_session_underline_type_t net_smux_session_underline_type(net_smux_session_t session);
net_address_t net_smux_session_local_address(net_smux_session_t session);

net_smux_stream_t net_smux_session_open_stream(net_smux_session_t session);

void net_smux_session_print(write_stream_t ws, net_smux_session_t session);
const char * net_smux_session_dump(mem_buffer_t buffer, net_smux_session_t session);

const char * net_smux_session_runing_mode_str(net_smux_session_runing_mode_t runing_mode);

NET_END_DECL

#endif
