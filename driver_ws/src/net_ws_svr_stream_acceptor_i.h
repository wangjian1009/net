#ifndef NET_WS_SVR_STREAM_ACCEPTOR_I_H_INCLEDED
#define NET_WS_SVR_STREAM_ACCEPTOR_I_H_INCLEDED
#include "net_ws_svr_stream_driver_i.h"

NET_BEGIN_DECL

struct net_ws_svr_stream_acceptor {
    net_acceptor_t m_underline;
};

int net_ws_svr_stream_acceptor_init(net_acceptor_t acceptor);
void net_ws_svr_stream_acceptor_fini(net_acceptor_t acceptor);

NET_END_DECL

#endif
