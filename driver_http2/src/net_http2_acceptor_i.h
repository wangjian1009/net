#ifndef NET_HTTP2_ACCEPTOR_I_H_INCLEDED
#define NET_HTTP2_ACCEPTOR_I_H_INCLEDED
#include "net_http2_acceptor.h"
#include "net_http2_driver_i.h"

struct net_http2_acceptor {
    net_acceptor_t m_base_acceptor;
};

int net_http2_acceptor_init(net_acceptor_t acceptor);
void net_http2_acceptor_fini(net_acceptor_t acceptor);

#endif
