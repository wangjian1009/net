#ifndef NET_HTTP_SVR_PROCESSOR_I_H
#define NET_HTTP_SVR_PROCESSOR_I_H
#include "net_http_svr_processor.h"
#include "net_http_svr_protocol_i.h"

struct net_http_svr_processor {
    net_http_svr_protocol_t m_service;
    TAILQ_ENTRY(net_http_svr_processor) m_next;
    char m_name[32];
    net_http_svr_mount_point_list_t m_mount_points;
    void * m_ctx;
    net_http_svr_processor_env_clear_fun_t m_env_clear;
    uint16_t m_request_sz;
    net_http_svr_processor_request_init_fun_t m_request_init;
    net_http_svr_processor_request_fini_fun_t m_request_fini;
    net_http_svr_processor_request_on_head_complete_fun_t m_request_on_head_complete;
    net_http_svr_processor_request_on_complete_fun_t m_request_on_complete;

    net_http_svr_request_list_t m_requests;
};

net_http_svr_processor_t net_http_svr_processor_create_root(net_http_svr_protocol_t service);

#endif
