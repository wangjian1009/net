#ifndef NET_NGHTTP2_SERVICE_I_H_INCLEDED
#define NET_NGHTTP2_SERVICE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_nghttp2_service.h"

typedef TAILQ_HEAD(net_nghttp2_connection_list, net_nghttp2_connection) net_nghttp2_connection_list_t;
typedef TAILQ_HEAD(net_nghttp2_request_list, net_nghttp2_request) net_nghttp2_request_list_t;
typedef TAILQ_HEAD(net_nghttp2_request_header_list, net_nghttp2_request_header) net_nghttp2_request_header_list_t;
typedef TAILQ_HEAD(net_nghttp2_response_list, net_nghttp2_response) net_nghttp2_response_list_t;
typedef TAILQ_HEAD(net_nghttp2_processor_list, net_nghttp2_processor) net_nghttp2_processor_list_t;
typedef TAILQ_HEAD(net_nghttp2_mount_point_list, net_nghttp2_mount_point) net_nghttp2_mount_point_list_t;

struct net_nghttp2_service {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    uint32_t m_cfg_connection_timeout_ms;

    net_nghttp2_processor_list_t m_processors;
    net_nghttp2_mount_point_t m_root;
    uint32_t m_request_sz;
    
    uint32_t m_max_request_id;
    net_nghttp2_connection_list_t m_connections;

    struct mem_buffer m_data_buffer; 
    struct mem_buffer m_search_path_buffer;
    
    net_nghttp2_request_list_t m_free_requests;
    net_nghttp2_request_header_list_t m_free_request_headers;
    net_nghttp2_response_list_t m_free_responses;
    net_nghttp2_mount_point_list_t m_free_mount_points;
};

mem_buffer_t net_nghttp2_service_tmp_buffer(net_nghttp2_service_t service);

#endif
