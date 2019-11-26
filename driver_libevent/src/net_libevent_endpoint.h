#ifndef NET_LIBEVENT_ENDPOINT_H_INCLEDED
#define NET_LIBEVENT_ENDPOINT_H_INCLEDED
#include "net_libevent_driver_i.h"

struct net_libevent_endpoint {
    struct bufferevent * m_bufferevent;
};

int net_libevent_endpoint_init(net_endpoint_t base_endpoint);
void net_libevent_endpoint_fini(net_endpoint_t base_endpoint);
int net_libevent_endpoint_connect(net_endpoint_t base_endpoint);
void net_libevent_endpoint_close(net_endpoint_t base_endpoint);
int net_libevent_endpoint_update(net_endpoint_t base_endpoint);

int net_libevent_endpoint_set_fd(net_libevent_endpoint_t endpoint, int fd);
int net_libevent_endpoint_set_established(net_libevent_endpoint_t endpoint);
int net_libevent_endpoint_update_local_address(net_libevent_endpoint_t endpoint);
int net_libevent_endpoint_update_remote_address(net_libevent_endpoint_t endpoint);

#endif
