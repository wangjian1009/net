#ifndef NET_SMUX_MANAGER_I_H_INCLEDED
#define NET_SMUX_MANAGER_I_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "cpe/utils/error.h"
#include "net_smux_manager.h"

NET_BEGIN_DECL

struct net_http_manager {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
};

mem_buffer_t net_http_protocol_tmp_buffer(net_smux_manager_t manager);

NET_END_DECL

#endif
