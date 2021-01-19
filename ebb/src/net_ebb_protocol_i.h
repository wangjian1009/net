#ifndef NET_EBB_SERVICE_H_I_INCLEDED
#define NET_EBB_SERVICE_H_I_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_ebb_protocol.h"

typedef TAILQ_HEAD(net_ebb_endpoint_list, net_ebb_endpoint) net_ebb_endpoint_list_t;
typedef TAILQ_HEAD(net_ebb_request_list, net_ebb_request) net_ebb_request_list_t;
typedef TAILQ_HEAD(net_ebb_request_header_list, net_ebb_request_header) net_ebb_request_header_list_t;
typedef TAILQ_HEAD(net_ebb_response_list, net_ebb_response) net_ebb_response_list_t;
typedef TAILQ_HEAD(net_ebb_processor_list, net_ebb_processor) net_ebb_processor_list_t;
typedef TAILQ_HEAD(net_ebb_mount_point_list, net_ebb_mount_point) net_ebb_mount_point_list_t;

struct net_ebb_protocol {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    uint32_t m_cfg_connection_timeout_ms;

    net_ebb_processor_list_t m_processors;
    net_ebb_mount_point_t m_root;
    uint32_t m_request_sz;
    
    uint32_t m_max_request_id;
    net_ebb_endpoint_list_t m_connections;

    struct mem_buffer m_data_buffer; 
    struct mem_buffer m_search_path_buffer;
    
    net_ebb_request_list_t m_free_requests;
    net_ebb_request_header_list_t m_free_request_headers;
    net_ebb_response_list_t m_free_responses;
    net_ebb_mount_point_list_t m_free_mount_points;
};

mem_buffer_t net_ebb_protocol_tmp_buffer(net_ebb_protocol_t service);

#endif
