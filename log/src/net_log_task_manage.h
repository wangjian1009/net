#ifndef NET_LOG_TASK_MANAGE_H_INCLEDED
#define NET_LOG_TASK_MANAGE_H_INCLEDED
#include "net_log_schedule_i.h"

struct net_log_task_manage {
    net_log_schedule_t m_schedule;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    net_log_task_list_t m_tasks;
    net_log_task_list_t m_free_tasks;
};

net_log_task_manage_t net_log_task_manage_create(net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver);
void net_log_task_manage_free(net_log_task_manage_t task_mgr);
    
#endif
