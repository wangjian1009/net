#ifndef NET_HTTP2_SVR_SERVICE_I_H_INCLEDED
#define NET_HTTP2_SVR_SERVICE_I_H_INCLEDED
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "nghttp2/nghttp2.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_http2_svr_service.h"

typedef TAILQ_HEAD(net_http2_svr_session_list, net_http2_svr_session) net_http2_svr_session_list_t;
typedef TAILQ_HEAD(net_http2_svr_request_list, net_http2_svr_request) net_http2_svr_request_list_t;
typedef TAILQ_HEAD(net_http2_svr_request_header_list, net_http2_svr_request_header) net_http2_svr_request_header_list_t;
typedef TAILQ_HEAD(net_http2_svr_response_list, net_http2_svr_response) net_http2_svr_response_list_t;
typedef TAILQ_HEAD(net_http2_svr_processor_list, net_http2_svr_processor) net_http2_svr_processor_list_t;
typedef TAILQ_HEAD(net_http2_svr_mount_point_list, net_http2_svr_mount_point) net_http2_svr_mount_point_list_t;

struct net_http2_svr_service {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    uint32_t m_cfg_session_timeout_ms;
    SSL_CTX * m_ssl_ctx;
    unsigned char m_next_proto_list[256];
    unsigned int m_next_proto_list_len;

    net_http2_svr_processor_list_t m_processors;
    net_http2_svr_mount_point_t m_root;
    uint32_t m_request_sz;
    
    uint32_t m_max_request_id;
    net_http2_svr_session_list_t m_sessions;

    struct mem_buffer m_data_buffer; 
    struct mem_buffer m_search_path_buffer;
    
    net_http2_svr_request_list_t m_free_requests;
    net_http2_svr_request_header_list_t m_free_request_headers;
    net_http2_svr_response_list_t m_free_responses;
    net_http2_svr_mount_point_list_t m_free_mount_points;
};

mem_buffer_t net_http2_svr_service_tmp_buffer(net_http2_svr_service_t service);

#endif
