#ifndef NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#define NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "net_ws_cli_protocol.h"

NET_BEGIN_DECL

struct net_ws_cli_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct mem_buffer m_data_buffer;
    /*protocol*/
    uint16_t m_protocol_capacity;
    net_ws_cli_protocol_init_fun_t m_protocol_init;
    net_ws_cli_protocol_fini_fun_t m_protocol_fini;
    /*endpoint*/
    uint16_t m_endpoint_capacity;
    net_ws_cli_endpoint_init_fun_t m_endpoint_init;
    net_ws_cli_endpoint_fini_fun_t m_endpoint_fini;
    net_ws_cli_endpoint_on_state_change_fun_t m_endpoint_on_state_change;
    net_ws_cli_endpoint_on_text_msg_fun_t m_endpoint_on_text_msg;
    net_ws_cli_endpoint_on_bin_msg_fun_t m_endpoint_on_bin_msg;
};

mem_buffer_t net_ws_cli_protocol_tmp_buffer(net_ws_cli_protocol_t ws_protocol);
net_schedule_t net_ws_cli_protocol_schedule(net_ws_cli_protocol_t ws_protocol);

NET_END_DECL

#endif
