#ifndef NET_WS_CLI_STREAM_ACCEPTOR_I_H_INCLEDED
#define NET_WS_CLI_STREAM_ACCEPTOR_I_H_INCLEDED
#include "net_ws_stream_acceptor.h"
#include "net_ws_stream_driver_i.h"

struct net_ws_stream_acceptor {
    net_acceptor_t m_base_acceptor;
};

int net_ws_stream_acceptor_init(net_acceptor_t acceptor);
void net_ws_stream_acceptor_fini(net_acceptor_t acceptor);

#endif
