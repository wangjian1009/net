#ifndef NET_XKCP_CLIENT_I_H_INCLEDED
#define NET_XKCP_CLIENT_I_H_INCLEDED
#include "net_xkcp_client.h"
#include "net_xkcp_acceptor_i.h"

struct net_xkcp_client {
    net_xkcp_acceptor_t m_acceptor;
    cpe_hash_entry m_hh_for_acceptor;
    net_address_t m_remote_address;
    net_timer_t m_timeout_timer;
    struct cpe_hash_table m_streams;
};

net_xkcp_client_t net_xkcp_client_create(net_xkcp_acceptor_t acceptor, net_address_t remote_address);
void net_xkcp_client_free(net_xkcp_client_t client);

int net_xkcp_client_eq(net_xkcp_client_t l, net_xkcp_client_t r, void * user_data);
uint32_t net_xkcp_client_hash(net_xkcp_client_t o, void * user_data);

#endif
