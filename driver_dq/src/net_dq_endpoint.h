#ifndef NET_DQ_ENDPOINT_H_INCLEDED
#define NET_DQ_ENDPOINT_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_dq_driver_i.h"

struct net_dq_endpoint {
    int m_fd;
    __unsafe_unretained dispatch_source_t m_source_r;
    __unsafe_unretained dispatch_source_t m_source_w;
};

int net_dq_endpoint_init(net_endpoint_t base_endpoint);
void net_dq_endpoint_fini(net_endpoint_t base_endpoint);
int net_dq_endpoint_connect(net_endpoint_t base_endpoint);
void net_dq_endpoint_close(net_endpoint_t base_endpoint);
int net_dq_endpoint_on_output(net_endpoint_t base_endpoint);

void net_dq_endpoint_start_rw_watcher(
    net_dq_driver_t driver, net_endpoint_t base_endpoint, net_dq_endpoint_t endpoint);

int net_dq_endpoint_update_local_address(net_dq_endpoint_t endpoint);
int net_dq_endpoint_update_remote_address(net_dq_endpoint_t endpoint);

#endif
