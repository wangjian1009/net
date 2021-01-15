#ifndef NET_SSL_CLI_PROTOCOL_I_H_INCLEDED
#define NET_SSL_CLI_PROTOCOL_I_H_INCLEDED
#include "net_ssl_cli_protocol.h"

struct net_ssl_cli_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
};

#endif
