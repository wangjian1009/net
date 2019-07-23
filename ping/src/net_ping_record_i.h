#ifndef NET_PING_RECORD_I_H_INCLEDED
#define NET_PING_RECORD_I_H_INCLEDED
#include "net_ping_record.h"
#include "net_ping_task_i.h"

struct net_ping_record {
    net_ping_task_t m_task;
    TAILQ_ENTRY(net_ping_record) m_next;
    uint32_t m_bytes;
    uint32_t m_value;
    uint16_t m_ttl;
};

net_ping_record_t net_ping_record_create(net_ping_task_t task);
void net_ping_record_free(net_ping_record_t record);
void net_ping_record_real_free(net_ping_record_t record);

#endif

