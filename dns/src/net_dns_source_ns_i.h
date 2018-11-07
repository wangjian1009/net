#ifndef NET_DNS_SOURCE_NS_I_H_INCLEDED
#define NET_DNS_SOURCE_NS_I_H_INCLEDED
#include "net_dns_source_ns.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_source_ns {
    net_driver_t m_driver;
    net_address_t m_address;
    net_dns_ns_trans_type_t m_trans_type;
    net_endpoint_prepare_connect_fun_t m_tcp_connect;
    void * m_tcp_connect_ctx;
    uint16_t m_timeout_ms;
    uint16_t m_retry_count;
    uint16_t m_max_transaction;
};

struct net_dns_source_ns_ctx {
    uint16_t m_transaction;
    union {
        net_dgram_t m_dgram;
        net_dns_ns_cli_endpoint_t m_tcp_cli;
    };
};

NET_END_DECL

#endif
