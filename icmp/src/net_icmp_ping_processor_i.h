#ifndef NET_ICMP_PING_PROCESSOR_I_H_INCLEDED
#define NET_ICMP_PING_PROCESSOR_I_H_INCLEDED
#include "net_icmp_ping_task_i.h"

struct net_icmp_ping_processor {
    net_icmp_ping_task_t m_task;
    TAILQ_ENTRY(net_icmp_ping_processor) m_next;
    net_address_t m_target;
    int m_fd;
    net_watcher_t m_watcher;
    uint16_t m_ping_id;
    uint16_t m_ping_index;
    uint16_t m_ping_count;
};

net_icmp_ping_processor_t
net_icmp_ping_processor_create(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count);
void net_icmp_ping_processor_free(net_icmp_ping_processor_t processor);
void net_icmp_ping_processor_real_free(net_icmp_ping_processor_t processor);

int net_icmp_ping_processor_start(net_icmp_ping_processor_t processor);

#endif

