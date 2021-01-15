#ifndef NET_SSL_CLI_DRIVER_I_H_INCLEDED
#define NET_SSL_CLI_DRIVER_I_H_INCLEDED
#include "net_ssl_cli_driver.h"

struct net_ssl_cli_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
};

#endif
