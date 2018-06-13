#ifndef NET_DNS_DGRAM_RECEIVER_H_INCLEDED
#define NET_DNS_DGRAM_RECEIVER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

typedef void (*net_dns_dgram_process_fun_t)(void * ctx, void * data, size_t data_size);

net_dns_dgram_receiver_t
net_dns_dgram_receiver_create(
    net_dns_manage_t manage,
    uint8_t address_is_own, net_address_t address,
    void * process_ctx, net_dns_dgram_process_fun_t process_fun);

void net_dns_dgram_receiver_free(net_dns_dgram_receiver_t receiver);

net_dns_dgram_receiver_t
net_dns_dgram_receiver_find_by_address(net_dns_manage_t manage, net_address_t address);

net_dns_dgram_receiver_t
net_dns_dgram_receiver_find_by_ctx(net_dns_manage_t manage, void * process_ctx, net_dns_dgram_process_fun_t process_fun);

NET_END_DECL

#endif
