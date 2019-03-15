#ifndef NET_ENDPOINT_WRITER_H_INCLEDED
#define NET_ENDPOINT_WRITER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

struct net_endpoint_writer {
    net_endpoint_t m_o_ep;
    net_endpoint_buf_type_t m_o_buf;
    uint8_t * m_wp;
    uint32_t m_capacity;
    uint32_t m_size;
    int m_totall_len;
};

int net_endpoint_writer_init(net_endpoint_writer_t writer, net_endpoint_t o_ep, net_endpoint_buf_type_t o_buf);
void net_endpoint_writer_cancel(net_endpoint_writer_t writer);
int net_endpoint_writer_commit(net_endpoint_writer_t writer);

int net_endpoint_writer_(net_endpoint_writer_t writer, void const * data, uint32_t sz);


NET_END_DECL

#endif
