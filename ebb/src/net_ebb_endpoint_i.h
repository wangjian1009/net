#ifndef NET_EBB_CONNECTION_H_I_INCLEDED
#define NET_EBB_CONNECTION_H_I_INCLEDED
#include "net_ebb_endpoint.h"
#include "net_ebb_protocol_i.h"
#include "net_ebb_request_parser.h"

struct net_ebb_endpoint {
    TAILQ_ENTRY(net_ebb_endpoint) m_next;
    net_timer_t m_timer_timeout;
    net_timer_t m_timer_close;
    //net_ebb_after_write_cb after_write_cb; /* ro */
    struct net_ebb_request_parser parser; /* private */
    net_ebb_request_list_t m_requests;
};

int net_ebb_endpoint_init(net_endpoint_t endpoint);
void net_ebb_endpoint_fini(net_endpoint_t endpoint);
int net_ebb_endpoint_input(net_endpoint_t endpoint);

void net_ebb_endpoint_timeout_reset(net_ebb_endpoint_t connection);
void net_ebb_endpoint_schedule_close(net_ebb_endpoint_t connection);
void net_ebb_endpoint_check_remove_done_requests(net_ebb_endpoint_t connection);

#endif
