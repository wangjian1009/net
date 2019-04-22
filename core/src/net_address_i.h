#ifndef NET_ADDRESS_I_H_INCLEDED
#define NET_ADDRESS_I_H_INCLEDED
#include "net_address.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

#define NET_ADDRESS_HEAD \
    net_schedule_t m_schedule; \
    net_address_type_t m_type; \
    uint16_t m_port; \

struct net_address {
    NET_ADDRESS_HEAD
};
    
struct net_address_ipv4v6 {
    NET_ADDRESS_HEAD
    union {
        struct net_address_data_ipv4 m_ipv4;
        struct net_address_data_ipv6 m_ipv6;
    };
};

struct net_address_domain {
    NET_ADDRESS_HEAD
    net_address_t m_resolved;
    char m_url[0];
};

struct net_address_local {
    NET_ADDRESS_HEAD
    char m_path[0];
};

struct net_address_in_cache {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_address_in_cache) m_next;
};

void net_address_real_free(net_address_in_cache_t address);

NET_END_DECL

#endif
