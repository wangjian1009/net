#include "cpe/pal/pal_socket.h"
#include "net_trans_task.h"
#include "net_ping_processor_i.h"

int net_ping_processor_start_http(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    //processor->m_http.m_task = net_trans_task_create(
    return 0;
}
