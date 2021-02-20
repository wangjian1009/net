#ifndef NET_XKCP_CLIENT_I_H_INCLEDED
#define NET_XKCP_CLIENT_I_H_INCLEDED
#include "net_xkcp_client.h"
#include "net_xkcp_acceptor_i.h"

struct net_xkcp_client {
    net_xkcp_acceptor_t m_acceptor;
    net_address_t m_remote_address;
    cpe_hash_entry m_hh_for_acceptor;
    struct cpe_hash_table m_streams;
};

#endif
