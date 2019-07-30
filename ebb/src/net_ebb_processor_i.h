#ifndef NET_EBB_PROCESSOR_I_H
#define NET_EBB_PROCESSOR_I_H
#include "net_ebb_processor.h"
#include "net_ebb_service_i.h"

struct net_ebb_processor {
    net_ebb_service_t m_service;
    TAILQ_ENTRY(net_ebb_processor) m_next;
    char m_name[32];
    net_ebb_mount_point_list_t m_mount_points;
    void * m_ctx;
    net_ebb_processor_env_clear_fun_t m_env_clear;

    net_ebb_request_list_t m_requests;
};

#endif
