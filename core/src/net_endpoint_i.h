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
    net_dns_query_t m_dns_query;
    ringbuffer_block_t m_rb;
    ringbuffer_block_t m_wb;
    ringbuffer_block_t m_fb;
    net_endpoint_monitor_list_t m_monitors;
};

void net_endpoint_real_free(net_endpoint_t endpoint);

void net_endpoint_notify_state_changed(net_endpoint_t endpoint, net_endpoint_state_t old_state);

ringbuffer_block_t net_endpoint_common_buf_alloc(net_endpoint_t endpoint, uint32_t size);

uint32_t net_endpoint_hash(net_endpoint_t o, void * user_data);
int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r, void * user_data);

NET_END_DECL

#endif
