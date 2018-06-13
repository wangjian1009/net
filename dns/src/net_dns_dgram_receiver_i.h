#ifndef NET_DNS_DYRAM_RECEIVER_I_H_INCLEDED
#define NET_DNS_DYRAM_RECEIVER_I_H_INCLEDED
#include "net_dns_dgram_receiver.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_dgram_receiver {
    net_dns_manage_t m_manage;
    uint8_t m_address_is_own;
    net_address_t m_address;
    struct cpe_hash_entry m_hh;
    void * m_process_ctx;
    net_dns_dgram_process_fun_t m_process_fun;
};

void net_dns_dgram_receiver_free_all(net_dns_manage_t manage);

uint32_t net_dns_dgram_receiver_hash(net_dns_dgram_receiver_t o, void * user_data);
int net_dns_dgram_receiver_eq(net_dns_dgram_receiver_t l, net_dns_dgram_receiver_t r, void * user_data);

NET_END_DECL

#endif

