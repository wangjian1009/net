#ifndef NET_ENDPOINT_I_H_INCLEDED
#define NET_ENDPOINT_I_H_INCLEDED
#include "net_endpoint.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_endpoint {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_endpoint) m_next_for_driver;
    net_address_t m_address;
    net_address_t m_remote_address;
    net_protocol_t m_protocol;
    net_endpoint_prepare_connect_fun_t m_prepare_connect;
    void * m_prepare_connect_ctx;
    net_link_t m_link;
    uint32_t m_id;
    uint8_t m_close_after_send;
    uint8_t m_protocol_debug;
    uint8_t m_driver_debug;
    net_endpoint_error_source_t m_error_source;
    uint32_t m_error_no;
    struct cpe_hash_entry m_hh;
    net_endpoint_state_t m_state;
    net_dns_query_t m_dns_query;
    ringbuffer_block_t m_bufs[net_ep_buf_count];
    void * m_data_watcher_ctx;
    net_endpoint_data_watch_fun_t m_data_watcher_fun;
    void (*m_data_watcher_fini)(void*, net_endpoint_t endpoint);
    net_endpoint_monitor_list_t m_monitors;
};

void net_endpoint_real_free(net_endpoint_t endpoint);

ringbuffer_block_t net_endpoint_common_buf_alloc(net_endpoint_t endpoint, uint32_t size);

uint32_t net_endpoint_hash(net_endpoint_t o, void * user_data);
int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r, void * user_data);

NET_END_DECL

#endif
