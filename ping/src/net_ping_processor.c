#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "net_watcher.h"
#include "net_trans_task.h"
#include "net_ping_processor_i.h"

net_ping_processor_t
net_ping_processor_create(net_ping_task_t task, uint16_t ping_count) {
    net_ping_mgr_t mgr = task->m_mgr;
    
    net_ping_processor_t processor = TAILQ_FIRST(&mgr->m_free_processors);
    if (processor) {
        TAILQ_REMOVE(&mgr->m_free_processors, processor, m_next);
    }
    else {
        processor = mem_alloc(mgr->m_alloc, sizeof(struct net_ping_processor));
        if (processor == NULL) {
            CPE_ERROR(
                mgr->m_em, "ping: %s: processor: alloc fail!", 
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
            return NULL;
        }
    }

    processor->m_task = task;
    
    switch(task->m_type) {
    case net_ping_type_icmp:
        processor->m_icmp.m_ping_id = ++mgr->m_icmp_ping_id_max;
        processor->m_icmp.m_fd = -1;
        processor->m_icmp.m_watcher = NULL;
        break;
    case net_ping_type_tcp_connect:
        break;
    case net_ping_type_http:
        processor->m_http.m_task = NULL;
        break;
    }
    
    processor->m_ping_index = 0;
    processor->m_ping_count = ping_count;
    
    return processor;
}

void net_ping_processor_free(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = processor->m_task->m_mgr;

    switch(task->m_type) {
    case net_ping_type_icmp:
        if (processor->m_icmp.m_watcher) {
            net_watcher_free(processor->m_icmp.m_watcher);
            processor->m_icmp.m_watcher = NULL;
        }

        if (processor->m_icmp.m_fd != -1) {
            cpe_sock_close(processor->m_icmp.m_fd);
            processor->m_icmp.m_fd = -1;
        }
        break;
    case net_ping_type_tcp_connect:
        break;
    case net_ping_type_http:
        if (processor->m_http.m_task) {
            net_trans_task_free(processor->m_http.m_task);
            processor->m_http.m_task = NULL;
        }
        break;
    }
    
    processor->m_task = (net_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_processors, processor, m_next);
}

void net_ping_processor_real_free(net_ping_processor_t processor) {
    net_ping_mgr_t mgr = (net_ping_mgr_t)processor->m_task;

    TAILQ_REMOVE(&mgr->m_free_processors, processor, m_next);
    
    mem_free(mgr->m_alloc, processor);
}
