#ifndef NET_PROXY_HTTP_SVR_PROTOCOL_I_H_INCLEDED
#define NET_PROXY_HTTP_SVR_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_proxy_http_svr_protocol.h"

NET_BEGIN_DECL

struct net_proxy_http_svr_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct mem_buffer m_data_buffer;
};

mem_buffer_t net_proxy_http_svr_protocol_tmp_buffer(net_proxy_http_svr_protocol_t http_protocol);
net_schedule_t net_proxy_http_svr_protocol_schedule(net_proxy_http_svr_protocol_t http_protocol);

NET_END_DECL

#endif
