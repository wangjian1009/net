#ifndef NET_SSL_SVR_UNDERLINE_I_H_INCLEDED
#define NET_SSL_SVR_UNDERLINE_I_H_INCLEDED
#include "net_ssl_svr_endpoint_i.h"

struct net_ssl_svr_undline {
    net_ssl_svr_endpoint_t m_ssl_endpoint;
};

net_protocol_t
net_ssl_svr_underline_protocol_create(net_schedule_t schedule, const char * name);

#endif
