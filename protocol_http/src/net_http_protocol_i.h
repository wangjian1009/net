#ifndef NET_HTTP_PROTOCOL_I_H_INCLEDED
#define NET_HTTP_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_http_protocol.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_http_req_list, net_http_req) net_http_req_list_t;

typedef enum net_http_trunked_state {
    net_http_trunked_length
    , net_http_trunked_content
    , net_http_trunked_content_complete
    , net_http_trunked_complete
} net_http_trunked_state_t;

struct net_http_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;

    /*runtime*/
    uint16_t m_max_req_id;
    net_http_req_list_t m_free_reqs;
    struct mem_buffer m_data_buffer;
};

mem_buffer_t net_http_protocol_tmp_buffer(net_http_protocol_t protocol);
net_schedule_t net_http_protocol_schedule(net_http_protocol_t protocol);

NET_END_DECL

#endif
