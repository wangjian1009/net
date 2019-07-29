#ifndef NET_EBB_CONNECTION_H_I_INCLEDED
#define NET_EBB_CONNECTION_H_I_INCLEDED
#include "net_ebb_connection.h"
#include "net_ebb_service_i.h"
#include "net_ebb_request_parser.h"

struct net_ebb_connection {
    TAILQ_ENTRY(net_ebb_connection) m_next;
    net_endpoint_t m_endpoint;
    net_timer_t m_timeout;
    //net_ebb_after_write_cb after_write_cb; /* ro */
    struct net_ebb_request_parser parser; /* private */

    /* Public */
    net_ebb_request_t (*new_request)(net_ebb_connection_t);

    void (*on_close)(net_ebb_connection_t);
};

int net_ebb_connection_init(net_endpoint_t endpoint);
void net_ebb_connection_fini(net_endpoint_t endpoint);
int net_ebb_connection_input(net_endpoint_t endpoint);

#endif
