#ifndef NET_LOG_REQUEST_MANAGE_H_INCLEDED
#define NET_LOG_REQUEST_MANAGE_H_INCLEDED
#include "net_log_schedule_i.h"

struct net_log_request_manage {
    net_log_schedule_t m_schedule;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    net_log_request_list_t m_requests;
    net_log_request_list_t m_free_requests;
};

net_log_request_manage_t net_log_request_manage_create(net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver);
void net_log_request_manage_free(net_log_request_manage_t request_mgr);
    
#endif
