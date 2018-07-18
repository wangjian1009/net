#ifndef NET_ADDRESS_H_INCLEDED
#define NET_ADDRESS_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

struct net_address_it {
    net_address_t (*next)(net_address_it_t it);
    char data[64];
};

struct net_address_data_ipv4 {
    union {
        uint8_t  u8[4];
        uint16_t  u16[2];
        uint32_t  u32;
    };
};

struct net_address_data_ipv6 {
    union {
        uint8_t  u8[16];
        uint16_t  u16[8];
        uint32_t  u32[4];
        uint64_t  u64[2];
    };
};

net_address_t net_address_create_ipv4(net_schedule_t schedule, const char * addr, uint16_t port);
net_address_t net_address_create_from_data_ipv4(net_schedule_t schedule, net_address_data_ipv4_t addr, uint16_t port);

net_address_t net_address_create_ipv6(net_schedule_t schedule, const char * addr, uint16_t port);
net_address_t net_address_create_from_data_ipv6(net_schedule_t schedule, net_address_data_ipv6_t addr, uint16_t port);

net_address_t net_address_create_domain(
    net_schedule_t schedule, const char * url, uint16_t port, net_address_t resolved);
net_address_t net_address_create_domain_with_len(
    net_schedule_t schedule, const void * url, uint16_t url_len, uint16_t port, net_address_t resolved);

net_address_t net_address_copy(net_schedule_t schedule, net_address_t from);

/*common*/
net_address_t net_address_create_from_sockaddr(net_schedule_t schedule, struct sockaddr * addr, socklen_t addr_len);
int net_address_to_sockaddr(net_address_t address, struct sockaddr * addr, socklen_t * addr_len);

net_address_type_t net_address_type(net_address_t address);
uint16_t net_address_port(net_address_t address);
void net_address_set_port(net_address_t address, uint16_t port);

void const * net_address_data(net_address_t address);

void net_address_print(write_stream_t ws, net_address_t address);
const char * net_address_dump(mem_buffer_t buffer, net_address_t address);

void net_address_free(net_address_t address);

int net_address_cmp(net_address_t l, net_address_t r);
int net_address_cmp_without_port(net_address_t l, net_address_t r);

net_address_t net_address_rand_same_network(net_address_t base_address, net_address_t mask);

/*domain*/
int net_address_set_resolved(net_address_t address, net_address_t resolved, uint8_t is_own);
net_address_t net_address_resolved(net_address_t address);

/**/
uint32_t net_address_hash(net_address_t address);

/**/
void net_address_it_init(net_address_it_t it);

#define net_address_it_next(__it) ((__it)->next(__it))

const char * net_address_type_str(net_address_type_t at);

NET_END_DECL

#endif
