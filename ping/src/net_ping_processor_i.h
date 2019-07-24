#ifndef NET_PING_PROCESSOR_I_H_INCLEDED
#define NET_PING_PROCESSOR_I_H_INCLEDED
#include "net_trans_system.h"
#include "net_ping_task_i.h"

struct net_ping_processor {
    net_ping_task_t m_task;
    TAILQ_ENTRY(net_ping_processor) m_next;
    union {
        struct {
            int m_fd;
            net_watcher_t m_watcher;
            uint16_t m_ping_id;
        } m_icmp;
        struct {
            int m_fd;
            net_watcher_t m_watcher;
        } m_tcp_connect;
        struct {
            net_trans_task_t m_task;
        } m_http;
    };
    int64_t m_start_time_ms;
    net_timer_t m_timer;
    uint16_t m_ping_count;
};

net_ping_processor_t
net_ping_processor_create(net_ping_task_t task, uint16_t ping_count);
void net_ping_processor_free(net_ping_processor_t processor);
void net_ping_processor_real_free(net_ping_processor_t processor);

int net_ping_processor_start(net_ping_processor_t processor);
int net_ping_processor_start_icmp(net_ping_processor_t processor);
int net_ping_processor_start_tcp_connect(net_ping_processor_t processor);
int net_ping_processor_start_http(net_ping_processor_t processor);

void net_point_processor_set_result_one(
    net_ping_processor_t processor, net_ping_error_t error, uint8_t error_go_on,
    uint32_t bytes, uint32_t ttl, uint32_t value);

#endif
