#ifndef NET_HTTP2_SVR_PROCESSOR_I_H
#define NET_HTTP2_SVR_PROCESSOR_I_H
#include "net_http2_svr_processor.h"
#include "net_http2_svr_service_i.h"

struct net_http2_svr_processor {
    net_http2_svr_service_t m_service;
    TAILQ_ENTRY(net_http2_svr_processor) m_next;
    char m_name[32];
    net_http2_svr_mount_point_list_t m_mount_points;
    void * m_ctx;
    net_http2_svr_processor_env_clear_fun_t m_env_clear;
    uint16_t m_request_sz;
    net_http2_svr_processor_request_init_fun_t m_request_init;
    net_http2_svr_processor_request_fini_fun_t m_request_fini;
    net_http2_svr_processor_request_on_head_complete_fun_t m_request_on_head_complete;
    net_http2_svr_processor_request_on_complete_fun_t m_request_on_complete;

    net_http2_svr_request_list_t m_requests;
};

net_http2_svr_processor_t net_http2_svr_processor_create_dft(net_http2_svr_service_t service);

#endif
