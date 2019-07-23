#include "net_address.h"
#include "net_icmp_ping_processor_i.h"
#include "net_icmp_pro.h"

net_icmp_ping_processor_t
net_icmp_ping_processor_create(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count) {
    net_icmp_ping_processor_t processor = mem_alloc(task->m_alloc, sizeof(struct net_icmp_ping_processor));
    if (processor == NULL) {
        CPE_ERROR(task->m_em, "icmp: ping: processor: alloc fail!");
        return NULL;
    }
    
    processor->m_task = task;
    processor->m_target = net_address_copy(task->m_schedule, target);
    processor->m_ping_count = ping_count;
    processor->m_fd = -1;

    return processor;
}

void net_icmp_ping_processor_free(net_icmp_ping_processor_t processor) {
    
}
