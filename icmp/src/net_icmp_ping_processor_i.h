#ifndef NET_ICMP_PING_PROCESSOR_I_H_INCLEDED
#define NET_ICMP_PING_PROCESSOR_I_H_INCLEDED
#include "net_icmp_ping_task_i.h"

struct net_icmp_ping_processor {
    net_icmp_ping_task_t m_task;
    net_address_t m_target;
    uint16_t m_ping_count;
    int m_fd;
};

net_icmp_ping_processor_t net_icmp_ping_processor_create(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count);
void net_icmp_ping_processor_free(net_icmp_ping_processor_t processor);

#endif

