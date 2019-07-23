#include "cpe/pal/pal_socket.h"
#include "net_address.h"
#include "net_watcher.h"
#include "net_ping_processor_i.h"

int net_ping_processor_start_icmp(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    return 0;
}
