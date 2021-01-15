#ifndef NET_SSL_SVR_ACCEPTOR_I_H_INCLEDED
#define NET_SSL_SVR_ACCEPTOR_I_H_INCLEDED
#include "net_ssl_svr_driver_i.h"

NET_BEGIN_DECL

struct net_ssl_svr_acceptor {
    net_acceptor_t m_underline;
};

int net_ssl_svr_acceptor_init(net_acceptor_t acceptor);
void net_ssl_svr_acceptor_fini(net_acceptor_t acceptor);

NET_END_DECL

#endif
