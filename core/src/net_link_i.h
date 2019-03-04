#ifndef NET_LINK_I_H_INCLEDED
#define NET_LINK_I_H_INCLEDED
#include "net_link.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_link {
    TAILQ_ENTRY(net_link) m_next;
    uint8_t m_local_is_tie;
    net_endpoint_t m_local;
    uint8_t m_remote_is_tie;
    net_endpoint_t m_remote;
    uint32_t m_buf_limit;
};

void net_link_real_free(net_link_t link);

NET_END_DECL

#endif
