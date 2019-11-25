#ifndef NET_NGHTTP2_PROCESSOR_I_H
#define NET_NGHTTP2_PROCESSOR_I_H
#include "net_nghttp2_processor.h"
#include "net_nghttp2_service_i.h"

struct net_nghttp2_processor {
    net_nghttp2_service_t m_service;
    TAILQ_ENTRY(net_nghttp2_processor) m_next;
    char m_name[32];
    net_nghttp2_mount_point_list_t m_mount_points;
    void * m_ctx;
    net_nghttp2_processor_env_clear_fun_t m_env_clear;
    uint16_t m_request_sz;
    net_nghttp2_processor_request_init_fun_t m_request_init;
    net_nghttp2_processor_request_fini_fun_t m_request_fini;
    net_nghttp2_processor_request_on_head_complete_fun_t m_request_on_head_complete;
    net_nghttp2_processor_request_on_complete_fun_t m_request_on_complete;

    net_nghttp2_request_list_t m_requests;
};

net_nghttp2_processor_t net_nghttp2_processor_create_dft(net_nghttp2_service_t service);

#endif
