#include "net_schedule.h"
#include "net_icmp_ping_task_i.h"
#include "net_icmp_ping_record_i.h"
#include "net_icmp_ping_processor_i.h"

net_icmp_ping_task_t net_icmp_ping_task_create(net_schedule_t schedule) {
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    error_monitor_t em = net_schedule_em(schedule);
    
    net_icmp_ping_task_t task = mem_alloc(alloc, sizeof(struct net_icmp_ping_task));
    if (task == NULL) {
        CPE_ERROR(em, "icmp: ping: allock fail!");
        return NULL;
    }

    task->m_alloc = alloc;
    task->m_em = em;
    task->m_schedule = schedule;
    task->m_state = net_icmp_ping_task_state_init;
    task->m_processor = NULL;
    TAILQ_INIT(&task->m_records);
    
    return task;
}

void net_icmp_ping_task_free(net_icmp_ping_task_t task) {
    while(!TAILQ_EMPTY(&task->m_records)) {
        net_icmp_ping_record_free(TAILQ_FIRST(&task->m_records));
    }
    
    if (task->m_processor) {
        net_icmp_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
    }
    
    mem_free(task->m_alloc, task);
}

net_icmp_ping_task_state_t net_icmp_ping_task_state(net_icmp_ping_task_t task) {
    return task->m_state;
}

void net_icmp_ping_task_records(net_icmp_ping_task_t task, net_icmp_ping_record_it_t record_it) {
}

int net_icmp_ping_task_start(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count) {
    return 0;
}

uint32_t net_icmp_ping_task_ping_max(net_icmp_ping_task_t task) {
    uint32_t max_ping = 0;

    net_icmp_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        if (record->m_value > max_ping) {
            max_ping = record->m_value;
        }
    }
    
    return max_ping;
}

uint32_t net_icmp_ping_task_ping_min(net_icmp_ping_task_t task) {
    uint32_t min_ping = 0;

    net_icmp_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        if (record->m_value < min_ping) {
            min_ping = record->m_value;
        }
    }
    
    return min_ping;
}

uint32_t net_icmp_ping_task_ping_avg(net_icmp_ping_task_t task) {
    uint32_t total_ping = 0;
    uint32_t count = 0;

    net_icmp_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        total_ping += record->m_value;
        count++;
    }
    
    return count > 0 ? (uint32_t)(total_ping / count) : 0;
}

const char * net_icmp_ping_task_state_str(net_icmp_ping_task_state_t state) {
    switch(state) {
    case net_icmp_ping_task_state_init:
        return "init";
    case net_icmp_ping_task_state_processing:
        return "processing";
    case net_icmp_ping_task_state_done:
        return "done";
    }
}
