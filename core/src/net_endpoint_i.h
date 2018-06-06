#ifndef NET_ENDPOINT_I_H_INCLEDED
#define NET_ENDPOINT_I_H_INCLEDED
#include "net_endpoint.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_endpoint {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_endpoint) m_next_for_driver;
    net_endpoint_type_t m_type;
    net_address_t m_address;
    net_address_t m_remote_address;
    net_protocol_t m_protocol;
    net_link_t m_link;
    uint32_t m_id;
    struct cpe_hash_entry m_hh;
    net_endpoint_state_t m_state;
    ringbuffer_block_t m_rb;
    ringbuffer_block_t m_wb;
    ringbuffer_block_t m_fb;
};

void net_endpoint_real_free(net_endpoint_t endpoint);

ringbuffer_block_t net_endpoint_common_buf_alloc(net_endpoint_t endpoint, uint32_t size);

uint32_t net_endpoint_hash(net_endpoint_t o);
int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r);

NET_END_DECL

#endif
