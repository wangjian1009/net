#ifndef NET_HTTP2_STREAM_GROUP_I_H_INCLEDED
#define NET_HTTP2_STREAM_GROUP_I_H_INCLEDED
#include "net_http2_stream_group.h"
#include "net_http2_protocol_i.h"
#include "net_http2_stream_driver_i.h"

struct net_http2_stream_group {
    net_http2_stream_driver_t m_driver;
    struct cpe_hash_entry m_hh_for_driver;
    net_address_t m_address;
    net_http2_endpoint_list_t m_endpoints;
};

int net_http2_stream_group_eq(net_http2_stream_group_t l, net_http2_stream_group_t r, void * user_data);
uint32_t net_http2_stream_group_hash(net_http2_stream_group_t o, void * user_data);

#endif    
