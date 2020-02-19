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
    TAILQ_ENTRY(net_endpoint) m_next_for_protocol;
    net_mem_group_t m_mem_group;
    net_endpoint_prepare_connect_fun_t m_prepare_connect;
    void * m_prepare_connect_ctx;
    net_link_t m_link;
    uint32_t m_id;
    uint32_t m_options;
    uint32_t m_dft_block_size;
    uint8_t m_expect_read;
    uint8_t m_is_writing;
    uint8_t m_close_after_send;
    uint8_t m_protocol_debug;
    uint8_t m_driver_debug;
    net_endpoint_error_source_t m_error_source;
    uint32_t m_error_no;
    char * m_error_msg;
    struct cpe_hash_entry m_hh;
    net_endpoint_state_t m_state;
    net_dns_query_t m_dns_query;
    net_mem_block_t m_tb;    
    struct {
        net_mem_block_list_t m_blocks;
        uint32_t m_size;
    } m_bufs[net_ep_buf_count];
    void * m_data_watcher_ctx;
    net_endpoint_data_watch_fun_t m_data_watcher_fun;
    net_endpoint_data_watch_ctx_fini_fun_t m_data_watcher_fini;
    net_endpoint_monitor_list_t m_monitors;
    net_endpoint_next_list_t m_nexts;
};

void net_endpoint_real_free(net_endpoint_t endpoint);

uint8_t net_endpoint_buf_validate(net_endpoint_t endpoint, void const * buf, uint32_t capacity);
void net_endpoint_send_evt(net_endpoint_t endpoint, net_endpoint_monitor_evt_t evt);

uint32_t net_endpoint_hash(net_endpoint_t o, void * user_data);
int net_endpoint_eq(net_endpoint_t l, net_endpoint_t r, void * user_data);

NET_END_DECL

#endif
