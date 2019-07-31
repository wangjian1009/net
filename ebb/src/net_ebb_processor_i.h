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
    uint16_t m_request_sz;
    net_ebb_processor_request_init_fun_t m_request_init;
    net_ebb_processor_request_fini_fun_t m_request_fini;
    net_ebb_processor_request_on_head_complete_fun_t m_request_on_head_complete;
    net_ebb_processor_request_on_complete_fun_t m_request_on_complete;

    net_ebb_request_list_t m_requests;
};

net_ebb_processor_t net_ebb_processor_create_dft(net_ebb_service_t service);

#endif
