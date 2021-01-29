#ifndef NET_WS_CLI_DRIVER_I_H_INCLEDED
#define NET_WS_CLI_DRIVER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "net_ws_cli_driver.h"

typedef struct net_ws_cli_underline * net_ws_cli_underline_t;
typedef struct net_ws_cli_underline_protocol * net_ws_cli_underline_protocol_t;

struct net_ws_cli_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
    net_protocol_t m_underline_protocol;
};

mem_buffer_t net_ws_cli_driver_tmp_buffer(net_ws_cli_driver_t driver);

#endif
