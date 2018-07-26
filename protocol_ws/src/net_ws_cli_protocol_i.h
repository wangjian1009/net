#ifndef NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#define NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#include "net_ws_cli_protocol.h"

NET_BEGIN_DECL

struct net_ws_cli_protocol {
    /*protocol*/
    uint16_t m_protocol_capacity;
    net_ws_cli_protocol_init_fun_t m_protocol_init;
    net_ws_cli_protocol_fini_fun_t m_protocol_fini;
    /*endpoint*/
    uint16_t m_endpoint_capacity;
    net_ws_cli_endpoint_init_fun_t m_endpoint_init;
    net_ws_cli_endpoint_fini_fun_t m_endpoint_fini;
};

NET_END_DECL

#endif
